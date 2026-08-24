#include "Button.h"
#include "../view/MotionEvent.h"
#include "../ui/TypedArray.h"
#include "../utils/Logger.h"
#include "../view/Gravity.h"
#include "../graphics/drawable/ColorDrawable.h"
#include "../graphics/drawable/StateListDrawable.h"
#include "../graphics/drawable/StateSet.h"

namespace setu {
namespace widget {

using view::Gravity;

// The chrome a Button adds around its label. These used to be applied twice over:
// once as a +32/+16 fudge in onMeasure and again as a canvas.translate() in
// onDraw. Real padding does the same job in one place, and it is what the
// gravity maths in TextView::onDraw needs in order to centre correctly.
static const int BUTTON_PADDING_H = 16;
static const int BUTTON_PADDING_V = 8;

// Only reached when nothing in the style names a background, which on a real
// device does not happen - every Button theme points at one.
static const uint32_t FALLBACK_BUTTON_COLOR = 0xFFDDDDDD;

namespace {

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
// colour of a <shape>'s solid or a TextView's label, whereas this feeds a
// hand-built selector of flat ColorDrawables standing in for btn_default's
// nine-patch assets. That stand-in retires with the bitmap pipeline (Phase 6),
// not before.
uint32_t fade(uint32_t argb, float factor) {
    const uint32_t a = (uint32_t)(((argb >> 24) & 0xFF) * factor);
    return (a << 24) | (argb & 0x00FFFFFFu);
}

// The <selector> a Button would have got from the framework, built by hand.
//
// Item order is AOSP's: the disabled item comes first so it wins even if a press
// flag is somehow still set, then pressed, then the resting item as the catch-all.
// A one-colour ColorDrawable cannot express any of this, which is why a flat
// colour from the style gets wrapped rather than installed as-is.
graphics::DrawablePtr makeDefaultBackground(uint32_t normalColor) {
    using namespace graphics::StateSet;

    auto selector = std::make_shared<graphics::StateListDrawable>();
    selector->addState({-STATE_ENABLED},
                       std::make_shared<graphics::ColorDrawable>(fade(normalColor, 0.38f)));
    selector->addState({STATE_PRESSED},
                       std::make_shared<graphics::ColorDrawable>(darken(normalColor, 0.72f)));
    selector->addState({}, std::make_shared<graphics::ColorDrawable>(normalColor));
    return selector;
}

} // namespace

Button::Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {

    // A Button centres its label, but android:gravity in the layout still wins.
    if (!mGravitySetFromXml) mGravity = Gravity::CENTER;
    // internalSetPadding, not setPadding: on a real device a Button's insets come
    // from its background's padding box, so a <shape> with <padding> from the
    // layout should replace these rather than be ignored. android:padding in the
    // layout still wins over both.
    internalSetPadding(BUTTON_PADDING_H, BUTTON_PADDING_V, BUTTON_PADDING_H, BUTTON_PADDING_V);

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
    internalSetPadding(BUTTON_PADDING_H, BUTTON_PADDING_V, BUTTON_PADDING_H, BUTTON_PADDING_V);

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
        // setPressed() refreshes the drawable state, which is what re-selects the
        // background's pressed item and repaints through Drawable::Callback. No
        // colour arithmetic and no window handle involved.
        setPressed(true);
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
