#include "TextView.h"
#include <algorithm>
#include <cmath>
#include "../ui/WindowManager.h"
#include <wrl/client.h>

namespace windroid {
namespace widget {

TextView::TextView() {
    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(16.0f);
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
    int desiredWidth = 0;
    int desiredHeight = 0;
    
    auto dWriteFactory = WindowManager::getDWriteFactory();
    if (dWriteFactory && !mText.empty()) {
        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
        dWriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            mTextPaint.getTextSize(),
            L"en-us",
            &textFormat
        );

        if (textFormat) {
            Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
            dWriteFactory->CreateTextLayout(
                mText.c_str(),
                (UINT32)mText.length(),
                textFormat.Get(),
                10000.0f, // Large max width for WRAP_CONTENT
                10000.0f, // Large max height for WRAP_CONTENT
                &textLayout
            );

            if (textLayout) {
                DWRITE_TEXT_METRICS metrics;
                textLayout->GetMetrics(&metrics);
                desiredWidth = (int)std::ceil(metrics.width);
                desiredHeight = (int)std::ceil(metrics.height);
            }
        }
    } else {
        // Fallback
        desiredWidth = (int)(mText.length() * (mTextPaint.getTextSize() * 0.6f)); 
        desiredHeight = (int)(mTextPaint.getTextSize() * 1.5f);
    }

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    int measuredWidth = desiredWidth;
    int measuredHeight = desiredHeight;

    if (widthMode == MEASURE_SPEC_EXACTLY) {
        measuredWidth = widthSize;
    } else if (widthMode == MEASURE_SPEC_AT_MOST) {
        measuredWidth = (std::min)(desiredWidth, widthSize);
    }

    if (heightMode == MEASURE_SPEC_EXACTLY) {
        measuredHeight = heightSize;
    } else if (heightMode == MEASURE_SPEC_AT_MOST) {
        measuredHeight = (std::min)(desiredHeight, heightSize);
    }

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void TextView::onDraw(graphics::Canvas& canvas) {
    // TODO: Add a debug mode in the future to toggle bounds visibility
    // graphics::Paint redPaint;
    // redPaint.setColor(0xFFFF0000); // Red
    // redPaint.setStyle(graphics::Style::FILL);
    // canvas.drawRect(0, 0, 50, 50, redPaint);

    if (!mText.empty()) {
        canvas.drawText(mText, 0, mTextPaint.getTextSize(), mTextPaint);
    }
}

} // namespace widget
} // namespace windroid
