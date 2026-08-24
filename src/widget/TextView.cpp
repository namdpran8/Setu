#include "TextView.h"

#include <algorithm>
#include <cmath>
#include <windows.h>

#include "../graphics/FontManager.h"
#include "../ui/TypedArray.h"
#include "../ui/WindowManager.h"
#include "../view/Gravity.h"

namespace setu {
namespace widget {

using view::Gravity;

static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

float TextView::getDefaultTextSizePx() {
    // AOSP's default is 14sp. The old hardcoded 16px was a raw pixel value, so
    // at our 2.0 scaled density every unstyled TextView rendered at what a real
    // device would show for 8sp - roughly half the size it should be.
    float scaledDensity = WindowManager::getScaledDensity();
    if (scaledDensity <= 0.0f) scaledDensity = 1.0f;
    return 14.0f * scaledDensity;
}

TextView::TextView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : View(resManager, theme, parser, defStyleAttr, defStyleRes) {

    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(getDefaultTextSizePx());
    mGravity = Gravity::TOP | Gravity::LEFT;

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
        if (a.hasValue(1)) setTextSize((float)a.getDimensionPixelSize(1, (int)getDefaultTextSizePx()));
        if (a.hasValue(2)) {
            // A selector first: the framework's own textColor is one, so a stock
            // widget's label greys out on its own once this resolves. getColor()
            // on the same value would flatten it to the default state's colour
            // and the disabled look would be lost.
            if (auto csl = a.getColorStateList(2)) {
                setTextColor(csl);
            } else {
                setTextColor(a.getColor(2, 0xFF000000));
            }
            mTextColorSetFromXml = true;
        }
        if (a.hasValue(3)) {
            mGravity = a.getInt(3, Gravity::TOP | Gravity::LEFT);
            mGravitySetFromXml = true;
        }
        if (a.hasValue(4)) mEms = a.getInt(4, -1);
    }
}

TextView::TextView() {
    mTextPaint.setColor(0xFF000000); // Black by default
    mTextPaint.setTextSize(getDefaultTextSizePx());
    mGravity = Gravity::TOP | Gravity::LEFT;
}

void TextView::setText(const std::wstring& text) {
    if (mText != text) {
        mText = text;
        mLayout.reset();
        invalidate();
        requestLayout();
    }
}

void TextView::setTextColor(uint32_t color) {
    // An explicit flat colour replaces any selector that was there. Written
    // straight into the paint rather than wrapped in a one-item list: AOSP does
    // wrap it, but that costs an allocation on a path this hot for no gain, and
    // the paint is what draws either way.
    mTextColorCsl.reset();
    if (mTextPaint.getColor() == color) return;
    mTextPaint.setColor(color);
    invalidate();  // colour does not affect line breaking, so no relayout
}

void TextView::setTextColor(const graphics::ColorStateListPtr& csl) {
    if (!csl) {
        mTextColorCsl.reset();
        return;
    }
    mTextColorCsl = csl;
    updateTextColors();
}

void TextView::updateTextColors() {
    if (!mTextColorCsl) return;

    // AOSP's fallback here is 0, but a text colour that resolves to transparent
    // is an invisible label, and a selector missing a wildcard item is a resource
    // bug rather than an instruction to hide the text. Falling back to the list's
    // own default keeps the text readable, and for every well-formed selector -
    // one with a wildcard item - the two are the same value anyway.
    const uint32_t color =
        mTextColorCsl->getColorForState(getDrawableState(), mTextColorCsl->getDefaultColor());
    if (mTextPaint.getColor() == color) return;
    mTextPaint.setColor(color);
    invalidate();
}

void TextView::drawableStateChanged() {
    View::drawableStateChanged();
    // Only a stateful list can produce a different answer; a constant one would
    // re-resolve to the same colour on every press.
    if (mTextColorCsl && mTextColorCsl->isStateful()) {
        updateTextColors();
    }
}

void TextView::setTextSize(float size) {
    if (mTextPaint.getTextSize() == size) return;
    mTextPaint.setTextSize(size);
    mLayout.reset();
    invalidate();
    requestLayout();
}

std::shared_ptr<graphics::TextLayout> TextView::obtainLayout(float maxWidth) {
    if (mLayout && mLayout->matches(mText, mTextPaint.getTextSize(), maxWidth)) {
        return mLayout;
    }
    return graphics::FontManager::getInstance().getTextLayout(mText, mTextPaint, maxWidth);
}

void TextView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    const int widthMode = getMode(widthMeasureSpec);
    const int widthSize = getSize(widthMeasureSpec);
    const int heightMode = getMode(heightMeasureSpec);
    const int heightSize = getSize(heightMeasureSpec);

    const int horizontalPadding = mPaddingLeft + mPaddingRight;
    const int verticalPadding = mPaddingTop + mPaddingBottom;

    int width;
    if (widthMode == MEASURE_SPEC_EXACTLY) {
        width = widthSize;
    } else {
        // AOSP measures the *unwrapped* width first (Layout.getDesiredWidth) and
        // only then clamps it to the spec. Measuring an already-wrapped layout
        // instead reports the widest wrapped line, and the final layout built at
        // that narrower width re-breaks into even more lines - which is how text
        // ends up shorter than the space reserved for it.
        auto unwrapped = obtainLayout(graphics::FontManager::UNBOUNDED_WIDTH);

        int desired;
        if (mEms > 0) {
            // AOSP treats one em as the line height (TextView.onMeasure's EMS
            // branch), and setEms() pins both min and max width, so the view is
            // exactly this wide regardless of its content.
            desired = (int)std::ceil(mEms * unwrapped->getFontMetrics().getLineHeight()) + horizontalPadding;
        } else {
            desired = (int)std::ceil(unwrapped->getWidth()) + horizontalPadding;
        }

        desired = (std::max)(desired, mMinWidth);
        width = (widthMode == MEASURE_SPEC_AT_MOST) ? (std::min)(desired, widthSize) : desired;
    }

    // The layout that onDraw will walk, wrapped against the width we are
    // actually going to be laid out at.
    const float layoutWidth = (float)(std::max)(0, width - horizontalPadding);
    mLayout = obtainLayout(layoutWidth);

    int height = (int)std::ceil(mLayout->getHeight()) + verticalPadding;
    height = (std::max)(height, mMinHeight);
    if (heightMode == MEASURE_SPEC_EXACTLY) {
        height = heightSize;
    } else if (heightMode == MEASURE_SPEC_AT_MOST) {
        height = (std::min)(height, heightSize);
    }

    setMeasuredDimension(width, height);
}

void TextView::onDraw(graphics::Canvas& canvas) {
    if (mText.empty()) return;

    if (!mLayout) {
        // Drawn without a measure pass; fall back to the bounds we were given.
        mLayout = obtainLayout((float)(std::max)(0, getWidth() - mPaddingLeft - mPaddingRight));
    }

    const int gravity = Gravity::getAbsoluteGravity(mGravity);

    // The whole text block is positioned vertically once...
    const float blockTop = Gravity::applyVertical(gravity, (float)getHeight(), mLayout->getHeight(),
                                                  (float)mPaddingTop, (float)mPaddingBottom);

    for (int i = 0; i < mLayout->getLineCount(); ++i) {
        const graphics::TextLayout::Line& line = mLayout->getLine(i);
        const std::wstring lineText = mLayout->getLineText(i);
        if (lineText.empty()) continue;

        // ...and every line is aligned horizontally on its own, the way
        // android.text.Layout does it.
        const float x = Gravity::applyHorizontal(gravity, (float)getWidth(), line.width,
                                                 (float)mPaddingLeft, (float)mPaddingRight);
        canvas.drawText(lineText, x, blockTop + line.baseline, mTextPaint);
    }
}

} // namespace widget
} // namespace setu
