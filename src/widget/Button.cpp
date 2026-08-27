/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Button.h"
#include "../view/MotionEvent.h"
#include "../ui/TypedArray.h"
#include "../ui/WindowManager.h"
#include "../ui/XmlAttrs.h"
#include "../utils/Logger.h"
#include "../view/Gravity.h"
#include "../graphics/drawable/ColorDrawable.h"
#include "../graphics/drawable/GradientDrawable.h"
#include "../graphics/drawable/InsetDrawable.h"
#include "../graphics/drawable/StateListDrawable.h"
#include "../graphics/drawable/StateSet.h"

namespace setu {
namespace widget {

using view::Gravity;

// The chrome a Button adds around its label. These used to be applied twice over:
// once as a +32/+16 fudge in onMeasure and again as a canvas.translate() in
// onDraw. Real padding does the same job in one place, and it is what the
// gravity maths in TextView::onDraw needs in order to centre correctly.
//
// Widget.Material.Button itself carries no padding at all - on a real device a
// Button's insets are the padding box of its background. btn_default_material
// wraps btn_default_mtrl_shape, an <inset> around a <shape> whose <padding> is
// button_padding_horizontal_material (8dp) and button_padding_vertical_material
// -> control_padding_material (4dp). makeDefaultBackground reproduces that stack,
// so the padding below reaches the view the way it does on a device: through
// View::setBackground reading the background's padding box. It is still applied
// directly as well, as the fallback for the one case that box does not exist -
// a background named by the style that reports no padding of its own.
//
// dp rather than raw pixels: 16 and 8 are what 8dp and 4dp come to at the
// density 2.0 the runtime was developed against, and wrong at every other one.
static const float BUTTON_PADDING_H_DP = 8.0f;
static const float BUTTON_PADDING_V_DP = 4.0f;

// btn_default_mtrl_shape's <inset>: button_inset_horizontal_material, which is
// control_inset_material (4dp), and button_inset_vertical_material (6dp). Note
// the asymmetry - it is not 4dp on all four edges.
//
// This is the gap between the view's bounds and its background, and it is what
// keeps two adjacent buttons, or a button and the window edge, from touching
// without either one naming a margin. The view's own bounds - and so its touch
// target - are unaffected, which is the whole reason it is an inset rather than
// more padding.
static const float BUTTON_INSET_H_DP = 4.0f;
static const float BUTTON_INSET_V_DP = 6.0f;

// Widget.Material.Button: minWidth 88dip, minHeight 48dip. The height is
// Material's minimum touch target, so a one-word label still gets a full-size
// button rather than one shrunk to fit its text.
static const float BUTTON_MIN_WIDTH_DP = 88.0f;
static const float BUTTON_MIN_HEIGHT_DP = 48.0f;

// Only reached when nothing in the style names a background, which on a real
// device does not happen - every Button theme points at one.
static const uint32_t FALLBACK_BUTTON_COLOR = 0xFFDDDDDD;

namespace {

// A style dimension in dp against the display metrics, the same shape as
// TextView::getDefaultTextSizePx resolving its 14sp default. The guard covers a
// Button built before the host has published its metrics.
float displayDensity() {
    const float density = WindowManager::getDensity();
    return density > 0.0f ? density : 1.0f;
}

// AOSP's two roundings, kept apart deliberately: a <shape>'s <padding> and an
// <inset>'s insets are both read with getDimensionPixelOffset, which truncates,
// while View's minWidth and minHeight come from getDimensionPixelSize, which
// rounds. They agree at density 2.0; at 2.625 a 4dp inset is 10px and not 11.
int offsetPx(float dp) { return dimensionPixelOffset(dp * displayDensity()); }
int sizePx(float dp) { return dimensionPixelSize(dp * displayDensity()); }

// Stand-ins for the layers of a real button background. The framework's own
// btn_default is a <selector> over nine-patch assets, so these only have to be
// plausible for any starting colour rather than exact for one.
uint32_t darken(uint32_t argb, float factor) {
    const uint32_t a = (argb >> 24) & 0xFF;
    const uint32_t r = (uint32_t)(((argb >> 16) & 0xFF) * factor);
    const uint32_t g = (uint32_t)(((argb >> 8) & 0xFF) * factor);
    const uint32_t b = (uint32_t)((argb & 0xFF) * factor);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// Faded rather than recoloured: a disabled button on a real device is the same
// shape at lower contrast, and fading keeps that true whatever the normal colour
// happens to be. Phase 4's ColorStateList does not retire this: it drives the
// colour of a <shape>'s solid, whereas this picks a colour per <selector> item -
// three flat shapes standing in for btn_default's nine-patch assets. That
// stand-in retires with the bitmap pipeline (Phase 6), not before.
uint32_t fade(uint32_t argb, float factor) {
    const uint32_t a = (uint32_t)(((argb >> 24) & 0xFF) * factor);
    return (a << 24) | (argb & 0x00FFFFFFu);
}

// One state's layer of the default background: btn_default_mtrl_shape's <shape>,
// a plain rectangle in `color` carrying the shape's <padding>.
//
// A GradientDrawable rather than a ColorDrawable, even though a zero-radius
// rectangle with a solid fill paints identically: only this class can carry a
// padding box, and that box is what makes the whole stack report the right
// content insets to View::setBackground. A flat colour would leave the selector
// reporting no padding, the <inset> above it reporting only its own 4dp/6dp, and
// the label sitting flush against the edge of the shape it is meant to be inside.
graphics::DrawablePtr makeButtonShape(uint32_t color) {
    // Shape::RECTANGLE is the default, which is what <shape> without an
    // android:shape means too.
    auto shape = std::make_shared<graphics::GradientDrawable>();
    shape->setColor(color);
    shape->setPaddingInsets(offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP),
                            offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP));
    return shape;
}

// The <selector> a Button would have got from the framework, built by hand, in the
// <inset> that btn_default_mtrl_shape wraps its shape in.
//
// Item order is AOSP's: the disabled item comes first so it wins even if a press
// flag is somehow still set, then pressed, then the resting item as the catch-all.
// A one-colour ColorDrawable cannot express any of this, which is why a flat
// colour from the style gets wrapped rather than installed as-is.
//
// One inset around the whole selector rather than one per item, which the real
// resource would give: the inset is the same in every state, and this way the
// arithmetic happens once instead of three times. DrawableContainer reports the
// per-edge maximum padding across its children, so the padding box that comes
// out the top is the same either way.
graphics::DrawablePtr makeDefaultBackground(uint32_t normalColor) {
    using namespace graphics::StateSet;

    auto selector = std::make_shared<graphics::StateListDrawable>();
    selector->addState({-STATE_ENABLED}, makeButtonShape(fade(normalColor, 0.38f)));
    selector->addState({STATE_PRESSED}, makeButtonShape(darken(normalColor, 0.72f)));
    selector->addState({}, makeButtonShape(normalColor));

    return std::make_shared<graphics::InsetDrawable>(
        std::move(selector),
        offsetPx(BUTTON_INSET_H_DP), offsetPx(BUTTON_INSET_V_DP),
        offsetPx(BUTTON_INSET_H_DP), offsetPx(BUTTON_INSET_V_DP));
}

} // namespace

Button::Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {

    // A Button centres its label, but android:gravity in the layout still wins.
    if (!mGravitySetFromXml) mGravity = Gravity::CENTER;
    // internalSetPadding, not setPadding: on a real device a Button's insets come
    // from its background's padding box, so a <shape> with <padding> from the
    // layout should replace these rather than be ignored. android:padding in the
    // layout still wins over both - LayoutInflater applies it through setPadding
    // after construction, and that is what mUserPaddingDefined protects.
    //
    // The background installed below reports a padding box of its own (the shape's
    // <padding> plus the <inset>), so for the default background these values are
    // superseded a few lines later by the same total plus the insets. They matter
    // for the one case that box is absent: a style background that claims none.
    internalSetPadding(offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP),
                       offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP));
    // Widget.Material.Button's 88dip x 48dip floor. TextView::onMeasure already
    // clamps both its wrap-content width and its height against these, so there
    // is nothing to override here.
    setMinimumWidth(sizePx(BUTTON_MIN_WIDTH_DP));
    setMinimumHeight(sizePx(BUTTON_MIN_HEIGHT_DP));

    graphics::DrawablePtr styleBackground;
    if (resManager) {
        // Mock styleables (Android framework IDs for android:background)
        static const uint32_t attr_background = 0x010100d4;

        std::vector<uint32_t> styleables = { attr_background };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);

        // getDrawable, not getColor: a real Button gets its background from its
        // style, and that background is a drawable far more often than a colour -
        // which is exactly the case getColor used to throw away.
        if (a.hasValue(0)) styleBackground = a.getDrawable(0);
    }

    if (auto* asColor = dynamic_cast<graphics::ColorDrawable*>(styleBackground.get())) {
        // A flat colour is not a background a real Button ever has, so it becomes the
        // resting layer of a selector instead of the whole background. Without this,
        // a themed Button would look right at rest and do nothing at all on touch.
        setBackground(makeDefaultBackground(asColor->getColor()));
    } else if (styleBackground) {
        // A drawable of its own, very often already a <selector> - in which case the
        // state this widget pushes through setPressed() is all it needs, and adding
        // anything here would override the app's own design.
        setBackground(std::move(styleBackground));
    } else {
        setBackground(makeDefaultBackground(FALLBACK_BUTTON_COLOR));
    }
}

Button::Button() {
    // Material Design Purple Button styling
    mGravity = Gravity::CENTER;
    internalSetPadding(offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP),
                       offsetPx(BUTTON_PADDING_H_DP), offsetPx(BUTTON_PADDING_V_DP));
    setMinimumWidth(sizePx(BUTTON_MIN_WIDTH_DP));
    setMinimumHeight(sizePx(BUTTON_MIN_HEIGHT_DP));

    setBackground(makeDefaultBackground(0xFF6200EE)); // Material Purple

    // Default text color for button
    setTextColor(0xFFFFFFFF); // White text
}

bool Button::onTouchEvent(view::MotionEvent& event) {
    // A disabled button still swallows the touch rather than letting it fall
    // through to whatever is behind it, which is what a real clickable-but-disabled
    // View does. It just does not press, click, or repaint.
    if (!isEnabled()) return true;

    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        Logger::d("Button", "Action::DOWN received");
        // Before setPressed, not after: a <ripple> background starts expanding from
        // wherever its hotspot is at the moment the pressed state arrives, so the
        // other order would ripple from the centre of the button on every first
        // touch. ViewGroup::dispatchTouchEvent has already offset the event into
        // this view's coordinates, which is the space the background lives in.
        drawableHotspotChanged(event.getX(), event.getY());
        // setPressed() refreshes the drawable state, which is what re-selects the
        // background's pressed item and repaints through Drawable::Callback. No
        // colour arithmetic and no window handle involved.
        setPressed(true);
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::MOVE) {
        // A ripple that has not finished entering follows the finger. Harmless for
        // every other background: setHotspot is a no-op on all of them.
        drawableHotspotChanged(event.getX(), event.getY());
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::UP) {
        Logger::d("Button", "Action::UP received, performing click");
        setPressed(false);
        performClick();
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::CANCEL) {
        Logger::d("Button", "Action::CANCEL received");
        setPressed(false);
        return true;
    }
    return false;
}

} // namespace widget
} // namespace setu
