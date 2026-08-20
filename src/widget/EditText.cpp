#include "EditText.h"
#include "../view/MotionEvent.h"
#include "../ui/WindowManager.h"

#include "../ui/TypedArray.h"
#include "../view/KeyEvent.h"

namespace setu {
namespace widget {

EditText::EditText(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {
    
    mBackgroundPaint.setColor(0xFFF5F5F5); // Light Gray background
    mBackgroundPaint.setStyle(graphics::Style::FILL);
    
    mLinePaint.setColor(0xFF6200EE); // Purple underline
    mLinePaint.setStrokeWidth(2.0f);
    mLinePaint.setStyle(graphics::Style::STROKE);

    setTextColor(0xFF000000); 

    if (resManager) {
        // Just mock parsing background color here too
        static const uint32_t attr_background = 0x01010039;
        std::vector<uint32_t> styleables = { attr_background };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);
        
        if (a.hasValue(0)) mBackgroundPaint.setColor(a.getColor(0, 0xFFF5F5F5));
    }
}

EditText::EditText() {
    // Material Design EditText styling
    mBackgroundPaint.setColor(0xFFF5F5F5); // Light Gray background
    mBackgroundPaint.setStyle(graphics::Style::FILL);
    
    mLinePaint.setColor(0xFF6200EE); // Purple underline
    mLinePaint.setStrokeWidth(2.0f);
    mLinePaint.setStyle(graphics::Style::STROKE);

    // EditText text should be darker than normal TextView
    setTextColor(0xFF000000); 
}

void EditText::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    TextView::onMeasure(widthMeasureSpec, heightMeasureSpec);
    
    // Add some padding for the background and line
    int paddedWidth = getMeasuredWidth() + 16;
    int paddedHeight = getMeasuredHeight() + 24; // extra height for the line
    
    setMeasuredDimension(paddedWidth, paddedHeight);
}

void EditText::onDraw(graphics::Canvas& canvas) {
    // Draw background
    canvas.drawRoundRect(0, 0, (float)getWidth(), (float)getHeight(), 4.0f, 4.0f, mBackgroundPaint);
    
    // Draw text using TextView's implementation (with padding)
    canvas.save();
    canvas.translate(8.0f, 8.0f);
    TextView::onDraw(canvas);
    canvas.restore();
    
    // Draw underline (thicker if focused, but we will just draw it always for now)
    if (mIsFocused) {
        mLinePaint.setStrokeWidth(3.0f);
    } else {
        mLinePaint.setStrokeWidth(1.0f);
    }
    canvas.drawLine(0, (float)getHeight() - 2.0f, (float)getWidth(), (float)getHeight() - 2.0f, mLinePaint);
}

bool EditText::onTouchEvent(view::MotionEvent& event) {
    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        mIsFocused = true;
        InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
        return true;
    }
    return false;
}

bool EditText::onKeyEvent(const view::KeyEvent& event) {
    if (!mIsFocused) return false;

    if (event.getAction() == view::KeyEvent::Action::DOWN) {
        if (event.getKeyCode() == 8) { // Backspace
            std::wstring currentText = getText();
            if (!currentText.empty()) {
                currentText.pop_back();
                setText(currentText);
                InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
            }
            return true;
        } else if (event.getCharacter() >= 32 && event.getCharacter() <= 126) {
            std::wstring currentText = getText();
            currentText += std::wstring(1, event.getCharacter());
            setText(currentText);
            InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
            return true;
        }
    }
    return false;
}

} // namespace widget
} // namespace setu


