#include "Button.h"
#include "../view/MotionEvent.h"
#include <windows.h>
#include "../ui/TypedArray.h"
#include "../ui/WindowManager.h"
#include "../utils/Logger.h"

namespace setu {
namespace widget {

Button::Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {
    
    mGravity = 0x11; // Gravity::CENTER by default for Button

    mBackgroundPaint.setColor(0xFFDDDDDD); // Light gray default
    mBackgroundPaint.setStyle(graphics::Style::FILL);

    if (resManager) {
        // Mock styleables (Android framework IDs for android:background)
        static const uint32_t attr_background = 0x010100d4;
        
        std::vector<uint32_t> styleables = { attr_background };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);
        
        if (a.hasValue(0)) mBackgroundPaint.setColor(a.getColor(0, 0xFFDDDDDD));
    }
}

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
    // Draw button background (use drawRect instead of drawRoundRect to prevent sparkles when buttons are adjacent without margins)
    canvas.drawRect(0, 0, (float)getWidth(), (float)getHeight(), mBackgroundPaint);
    
    // Adjust canvas for padding before drawing text
    canvas.save();
    canvas.translate(16.0f, 8.0f); // 16px left padding, 8px top padding
    
    // Log text for debugging
    std::string textUtf8;
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, getText().c_str(), (int)getText().size(), NULL, 0, NULL, NULL);
    if (size_needed > 0) {
        textUtf8.resize(size_needed);
        WideCharToMultiByte(CP_UTF8, 0, getText().c_str(), (int)getText().size(), &textUtf8[0], size_needed, NULL, NULL);
        Logger::d("Button", "Drawing text: '" + textUtf8 + "' color=0x" + std::to_string(mTextPaint.getColor()) + 
                  " at (" + std::to_string((float)getLeft()) + "," + std::to_string((float)getTop()) + ")");
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
} // namespace setu


