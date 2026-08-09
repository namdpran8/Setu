#include "Button.h"
#include "../view/MotionEvent.h"

namespace windroid {
namespace widget {

Button::Button() {
    // Default button styling
    mBackgroundPaint.setColor(0xFFCCCCCC); // Light gray background
    mBackgroundPaint.setStyle(graphics::Style::FILL);
    
    // Default text color for button
    setTextColor(0xFF000000); // Black text
}

void Button::setOnClickListener(std::function<void()> listener) {
    mOnClickListener = listener;
}

void Button::performClick() {
    if (mOnClickListener) {
        mOnClickListener();
    }
}

void Button::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    TextView::onMeasure(widthMeasureSpec, heightMeasureSpec);
    
    // Add some padding for the button background
    int paddedWidth = getMeasuredWidth() + 32;
    int paddedHeight = getMeasuredHeight() + 16;
    
    setMeasuredDimension(paddedWidth, paddedHeight);
}

void Button::onDraw(graphics::Canvas& canvas) {
    // Draw button background
    canvas.drawRoundRect(0, 0, (float)getWidth(), (float)getHeight(), 8.0f, 8.0f, mBackgroundPaint);
    
    // Adjust canvas for padding before drawing text
    canvas.save();
    canvas.translate(16.0f, 8.0f); // 16px left padding, 8px top padding
    
    // Draw text using TextView's implementation
    TextView::onDraw(canvas);
    
    canvas.restore();
}

bool Button::onTouchEvent(view::MotionEvent& event) {
    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        mBackgroundPaint.setColor(0xFF999999); // Darker gray when pressed
        // Ideally we'd call invalidate() here to trigger a redraw
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::UP) {
        mBackgroundPaint.setColor(0xFFCCCCCC); // Restore original color
        performClick();
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::CANCEL) {
        mBackgroundPaint.setColor(0xFFCCCCCC); // Restore original color
        return true;
    }
    return false;
}

} // namespace widget
} // namespace windroid
