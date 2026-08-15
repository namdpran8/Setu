#include "TextView.h"
#include <algorithm>
#include <cmath>
#include <windows.h>
#include "../ui/WindowManager.h"
#include "../ui/TypedArray.h"
#include <wrl/client.h>

namespace setu {
namespace widget {

static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

TextView::TextView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : View(resManager, theme, parser, defStyleAttr, defStyleRes) {
    
    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(16.0f);
    mGravity = 0x33;

    if (resManager) {
        // Mock styleables (Android framework IDs for android:text, android:textSize, android:textColor, android:gravity)
        static const uint32_t attr_text = 0x0101014f;
        static const uint32_t attr_textSize = 0x01010095;
        static const uint32_t attr_textColor = 0x01010098;
        static const uint32_t attr_gravity = 0x010100af;
        
        std::vector<uint32_t> styleables = { attr_text, attr_textSize, attr_textColor, attr_gravity };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);
        
        if (a.hasValue(0)) setText(utf8_to_utf16(a.getString(0)));
        if (a.hasValue(1)) setTextSize((float)a.getDimensionPixelSize(1, 16));
        if (a.hasValue(2)) setTextColor(a.getColor(2, 0xFF000000));
        if (a.hasValue(3)) mGravity = a.getInt(3, 0x33);
    }
}

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
    if (!mText.empty()) {
        float x = 0;
        float y = mTextPaint.getTextSize();

        if (mGravity == 0x11) { // CENTER
            // Approximate centering based on bounds
            float approxTextWidth = mText.length() * (mTextPaint.getTextSize() * 0.6f);
            x = (getWidth() - approxTextWidth) / 2.0f;
            y = (getHeight() + mTextPaint.getTextSize()) / 2.0f - (mTextPaint.getTextSize() * 0.2f);
        }

        canvas.drawText(mText, x, y, mTextPaint);
    }
}

} // namespace widget
} // namespace setu


