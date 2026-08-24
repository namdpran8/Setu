#include "EditText.h"
#include "../view/MotionEvent.h"

#include "../graphics/drawable/ColorDrawable.h"
#include "../ui/TypedArray.h"
#include "../view/KeyEvent.h"

namespace setu {
namespace widget {

// Insets for the field's chrome. Previously these were a +16/+24 fudge in
// onMeasure plus a canvas.translate(8, 8) in onDraw; expressing them as real
// padding keeps the same geometry while letting TextView position the text.
// The bottom is deeper than the top to leave room for the underline.
static const int EDIT_PADDING_H = 8;
static const int EDIT_PADDING_TOP = 8;
static const int EDIT_PADDING_BOTTOM = 16;

// Raw pixels, not dp, to match the rest of this widget's stand-in chrome above.
// All of it goes away once the real Material EditText style is being read.
static const float EDIT_CORNER_RADIUS = 4.0f;

static const uint32_t EDIT_DEFAULT_BACKGROUND = 0xFFF5F5F5; // Light Gray

void EditText::installDefaultBackground(uint32_t color) {
    // The rounded fill used to be a drawRoundRect in onDraw, because a
    // ColorDrawable has square corners. GradientDrawable is what a real
    // <shape android:shape="rectangle"><corners/></shape> inflates to, so the
    // field now draws through exactly the same path a layout's background does.
    mDefaultBackground = std::make_shared<graphics::GradientDrawable>();
    mDefaultBackground->setShape(graphics::GradientDrawable::Shape::RECTANGLE);
    mDefaultBackground->setCornerRadius(EDIT_CORNER_RADIUS);
    mDefaultBackground->setColor(color);
    setBackground(mDefaultBackground);
}

EditText::EditText(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {

    // internalSetPadding, not setPadding: a real field's insets come from its
    // background's padding box, so a <shape> with <padding> from the layout
    // should replace these. android:padding still wins over both.
    internalSetPadding(EDIT_PADDING_H, EDIT_PADDING_TOP, EDIT_PADDING_H, EDIT_PADDING_BOTTOM);

    mLinePaint.setColor(0xFF6200EE); // Purple underline
    mLinePaint.setStrokeWidth(2.0f);
    mLinePaint.setStyle(graphics::Style::STROKE);

    // A stand-in for the textAppearance chain a real field inherits its colour
    // from, not an override of the layout. The base constructor has already run,
    // so applying this unconditionally would discard whatever android:textColor
    // named - a colour selector included.
    if (!mTextColorSetFromXml) setTextColor(0xFF000000);

    graphics::DrawablePtr styleBackground;
    if (resManager) {
        // Mock styleables (Android framework IDs for android:background)
        static const uint32_t attr_background = 0x010100d4;
        std::vector<uint32_t> styleables = { attr_background };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);

        if (a.hasValue(0)) styleBackground = a.getDrawable(0);
    }

    if (auto* asColor = dynamic_cast<graphics::ColorDrawable*>(styleBackground.get())) {
        // Recolour the shape rather than taking the ColorDrawable: the rounded
        // corners are part of the field's look, and a flat colour from the style
        // should not square them off.
        installDefaultBackground(asColor->getColor());
    } else if (styleBackground) {
        // A real drawable from the style describes the whole field, corners
        // included, so the stand-in has nothing left to say.
        mDefaultBackground.reset();
        setBackground(std::move(styleBackground));
    } else {
        installDefaultBackground(EDIT_DEFAULT_BACKGROUND);
    }
}

EditText::EditText() {
    internalSetPadding(EDIT_PADDING_H, EDIT_PADDING_TOP, EDIT_PADDING_H, EDIT_PADDING_BOTTOM);

    // Material Design EditText styling
    installDefaultBackground(EDIT_DEFAULT_BACKGROUND);

    mLinePaint.setColor(0xFF6200EE); // Purple underline
    mLinePaint.setStrokeWidth(2.0f);
    mLinePaint.setStyle(graphics::Style::STROKE);

    // EditText text should be darker than normal TextView
    setTextColor(0xFF000000);
}

void EditText::onDraw(graphics::Canvas& canvas) {
    // The fill is View's background drawable now, already painted by the time
    // onDraw runs.

    // TextView::onDraw honours our padding, so no translate is needed.
    TextView::onDraw(canvas);

    // Draw underline (thicker while focused)
    mLinePaint.setStrokeWidth(isFocused() ? 3.0f : 1.0f);
    canvas.drawLine(0, (float)getHeight() - 2.0f, (float)getWidth(), (float)getHeight() - 2.0f, mLinePaint);
}

bool EditText::onTouchEvent(view::MotionEvent& event) {
    // A disabled field consumes the touch but does not take focus, so it cannot end
    // up with a caret in it and no way to type. Matches a real device.
    if (!isEnabled()) return true;

    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        if (!isFocused()) {
            setFocus(true);
            // invalidate() now reaches the window on its own, so the underline
            // thickens without the widget touching the HWND.
            invalidate();
        }
        return true;
    }
    return false;
}

bool EditText::onKeyEvent(const view::KeyEvent& event) {
    // Focus can outlive being disabled - setEnabled(false) does not clear it on a
    // real device either - so this is checked on the way in rather than assumed.
    if (!isEnabled()) return false;
    if (!isFocused()) return false;

    if (event.getAction() == view::KeyEvent::Action::DOWN) {
        if (event.getKeyCode() == 8) { // Backspace
            std::wstring currentText = getText();
            if (!currentText.empty()) {
                currentText.pop_back();
                setText(currentText);
            }
            return true;
        } else if (event.getCharacter() >= 32 && event.getCharacter() <= 126) {
            std::wstring currentText = getText();
            currentText += std::wstring(1, event.getCharacter());
            setText(currentText);
            return true;
        }
    }
    return false;
}

} // namespace widget
} // namespace setu
