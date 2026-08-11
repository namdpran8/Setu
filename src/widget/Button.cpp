#include "Button.h"
#include "../view/MotionEvent.h"
#include <windows.h>
#include "../ui/WindowManager.h"
#include "../utils/Logger.h"

namespace windroid {
namespace widget {

Button::Button() {
    // Material Design Purple Button styling
    mBackgroundPaint.setColor(0xFF6200EE); // Material Purple
    mBackgroundPaint.setStyle(graphics::Style::FILL);
    
    // Default text color for button
    setTextColor(0xFFFFFFFF); // White text
}

void Button::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    TextView::onMeasure(widthMeasureSpec, heightMeasureSpec);
    
    int widthMode = View::getMode(widthMeasureSpec);
    int widthSize = View::getSize(widthMeasureSpec);
    int heightMode = View::getMode(heightMeasureSpec);
    int heightSize = View::getSize(heightMeasureSpec);
    
    // Add some padding for the button background
    int paddedWidth = getMeasuredWidth() + 32;
    int paddedHeight = getMeasuredHeight() + 16;
    
    if (widthMode == View::MEASURE_SPEC_EXACTLY) paddedWidth = widthSize;
    if (heightMode == View::MEASURE_SPEC_EXACTLY) paddedHeight = heightSize;
    
    setMeasuredDimension(paddedWidth, paddedHeight);
}

void Button::onDraw(graphics::Canvas& canvas) {
    // Draw button background
    canvas.drawRoundRect(0, 0, (float)getWidth(), (float)getHeight(), 8.0f, 8.0f, mBackgroundPaint);
    
    // Adjust canvas for padding before drawing text
    canvas.save();
    canvas.translate(16.0f, 8.0f); // 16px left padding, 8px top padding
    
    // Log text for debugging
    std::string textUtf8;
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, getText().c_str(), (int)getText().size(), NULL, 0, NULL, NULL);
    if (size_needed > 0) {
        textUtf8.resize(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, getText().c_str(), (int)getText().size(), &textUtf8[0], size_needed, NULL, NULL);
        Logger::d("Button", "Drawing text: '" + textUtf8 + "' at mLeft=" + std::to_string(getLeft()) + ", mTop=" + std::to_string(getTop()));
    }
    
    // Draw text using TextView's implementation
    TextView::onDraw(canvas);
    
    canvas.restore();
}

bool Button::onTouchEvent(view::MotionEvent& event) {
    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        mBackgroundPaint.setColor(0xFF3700B3); // Darker purple when pressed
        InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::UP) {
        mBackgroundPaint.setColor(0xFF6200EE); // Restore original purple
        InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
        performClick();
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::CANCEL) {
        mBackgroundPaint.setColor(0xFF6200EE); // Restore original purple
        InvalidateRect(WindowManager::getMainWindow(), nullptr, FALSE);
        return true;
    }
    return false;
}

} // namespace widget
} // namespace windroid
