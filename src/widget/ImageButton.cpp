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

#include "ImageButton.h"

#include <utility>
#include <vector>

#include "../ui/AndroidAttrs.h"
#include "../ui/TypedArray.h"
#include "../utils/Logger.h"
#include "../view/MotionEvent.h"

namespace setu {
namespace widget {

namespace {
constexpr const char* kTag = "ImageButton";
}

ImageButton::ImageButton(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser,
                         uint32_t defStyleAttr, uint32_t defStyleRes)
    : ImageView(resManager, theme, parser, defStyleAttr, defStyleRes) {
    // From Widget.ImageButton, not from ImageButton.java. LayoutInflater's generic
    // attribute loop runs after this constructor and applies the layout's own
    // android:clickable / android:focusable, so an explicit false there still wins.
    setFocusable(true);
    setClickable(true);

    // Also from the style, and only as a fallback: if imageButtonStyle resolved,
    // ImageView's constructor has already read scaleType=center out of it and set
    // the flag. Overwriting it unconditionally would discard an android:scaleType
    // the layout author actually wrote.
    if (!mScaleTypeSetFromXml) {
        setScaleType(ScaleType::CENTER);
    }

    if (!resManager) return;

    // android:background belongs to the View styleable, and View reads no
    // attributes at all here, so nothing else in the hierarchy will pick this up.
    // It is the whole visual identity of an ImageButton: btn_default is a
    // <selector> over nine-patch assets, and its padding rect is what insets the
    // icon from the button's edge - which View::setBackground applies for us.
    const uint32_t attrBackground = androidAttr(resManager, "background");

    std::vector<uint32_t> styleables = {attrBackground};
    TypedArray a(resManager, styleables);
    a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);

    graphics::DrawablePtr background = a.hasValue(0) ? a.getDrawable(0) : nullptr;
    if (background) {
        setBackground(std::move(background));
    } else {
        // No stand-in built here, deliberately. Button hand-assembles a selector of
        // flat ColorDrawables because a coloured rectangle is a passable stand-in
        // for a labelled button; an unlabelled icon button drawn as a grey box is
        // just a grey box. Better to show the icon on its own and say why.
        Logger::d(kTag, "no background resolved from the style; drawing the icon "
                        "with no button chrome");
    }
}

ImageButton::ImageButton() : ImageView() {
    setFocusable(true);
    setClickable(true);
    setScaleType(ScaleType::CENTER);
}

bool ImageButton::onTouchEvent(view::MotionEvent& event) {
    // Swallowed rather than passed through, matching a real disabled-but-clickable
    // View: it does not press, click or repaint, but neither does it let the touch
    // reach whatever is behind it.
    if (!isEnabled()) return true;

    switch (event.getAction()) {
        case view::MotionEvent::Action::DOWN:
            // Hotspot before setPressed, not after. A <ripple> background starts
            // expanding from wherever its hotspot sits at the moment the pressed
            // state arrives, so the other order ripples from the centre on every
            // first touch. ViewGroup::dispatchTouchEvent has already offset the
            // event into this view's coordinates, which is the space the background
            // lives in.
            drawableHotspotChanged(event.getX(), event.getY());
            setPressed(true);
            return true;

        case view::MotionEvent::Action::MOVE:
            // An in-flight ripple follows the finger; a no-op for every other kind
            // of background.
            drawableHotspotChanged(event.getX(), event.getY());
            return true;

        case view::MotionEvent::Action::UP:
            setPressed(false);
            performClick();
            return true;

        case view::MotionEvent::Action::CANCEL:
            setPressed(false);
            return true;

        default:
            return false;
    }
}

} // namespace widget
} // namespace setu
