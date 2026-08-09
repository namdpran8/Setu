#include "TextView.h"
#include <algorithm>

namespace windroid {
namespace widget {

TextView::TextView() {
    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(14.0f);
}

void TextView::setText(const std::wstring& text) {
    mText = text;
    // In a real framework, we would call requestLayout() here
}

void TextView::setTextColor(uint32_t color) {
    mTextPaint.setColor(color);
    // In a real framework, we would call invalidate() here
}

void TextView::setTextSize(float size) {
    mTextPaint.setTextSize(size);
}

void TextView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // Very naive measurement for Phase 2.
    // In Phase 3 (Text Measurement), we will query DirectWrite for actual bounds.
    int desiredWidth = (int)(mText.length() * (mTextPaint.getTextSize() * 0.6f)); 
    int desiredHeight = (int)(mTextPaint.getTextSize() * 1.5f);

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    int measuredWidth = desiredWidth;
    int measuredHeight = desiredHeight;

    if (widthMode == MEASURE_SPEC_EXACTLY) {
        measuredWidth = widthSize;
    } else if (widthMode == MEASURE_SPEC_AT_MOST) {
        measuredWidth = std::min(desiredWidth, widthSize);
    }

    if (heightMode == MEASURE_SPEC_EXACTLY) {
        measuredHeight = heightSize;
    } else if (heightMode == MEASURE_SPEC_AT_MOST) {
        measuredHeight = std::min(desiredHeight, heightSize);
    }

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void TextView::onDraw(graphics::Canvas& canvas) {
    // Draw the text
    // We adjust Y slightly downwards because Canvas drawText expects baseline in Android, 
    // but our Direct2D implementation handles it (roughly) for now.
    canvas.drawText(mText, 0, mTextPaint.getTextSize(), mTextPaint);
}

} // namespace widget
} // namespace windroid
