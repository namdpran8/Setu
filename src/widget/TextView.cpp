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
        static const uint32_t attr_ems = 0x01010158;
        
        std::vector<uint32_t> styleables = { attr_text, attr_textSize, attr_textColor, attr_gravity, attr_ems };
        TypedArray a(resManager, styleables);
        a.obtainStyledAttributes(theme, parser, defStyleAttr, defStyleRes);
        
        if (a.hasValue(0)) setText(utf8_to_utf16(a.getString(0)));
        if (a.hasValue(1)) setTextSize((float)a.getDimensionPixelSize(1, 16));
        if (a.hasValue(2)) setTextColor(a.getColor(2, 0xFF000000));
        if (a.hasValue(3)) mGravity = a.getInt(3, 0x33);
        if (a.hasValue(4)) mEms = a.getInt(4, -1);
    }
}

TextView::TextView() {
    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(16.0f);
}

void TextView::setText(const std::wstring& text) {
    if (mText != text) {
        mText = text;
        invalidate();
        requestLayout();
    }
}

void TextView::setTextColor(uint32_t color) {
    mTextPaint.setColor(color);
    invalidate();
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
        // GDI FALLBACK - EXACT MEASUREMENT
        HDC hdc = GetDC(nullptr);
        HFONT hFont = CreateFontW(
            -MulDiv((int)mTextPaint.getTextSize(), GetDeviceCaps(hdc, LOGPIXELSY), 72),
            0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ old = SelectObject(hdc, hFont);
        
        SIZE size;
        if (mText.empty()) {
            GetTextExtentPoint32W(hdc, L"A", 1, &size);
            size.cx = 0; // Empty text has 0 width but full font height
        } else {
            GetTextExtentPoint32W(hdc, mText.c_str(), (int)mText.length(), &size);
        }
        
        SelectObject(hdc, old);
        DeleteObject(hFont);
        ReleaseDC(nullptr, hdc);
        desiredWidth = size.cx + mPaddingLeft + mPaddingRight;
        desiredHeight = size.cy + mPaddingTop + mPaddingBottom;
    }

    if (mEms > 0) {
        desiredWidth = (int)(mTextPaint.getTextSize() * 0.6f * mEms) + mPaddingLeft + mPaddingRight;
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


