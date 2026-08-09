#include "Paint.h"

namespace windroid {
namespace graphics {

Paint::Paint()
    : mColor(0xFF000000), // Black, opaque
      mStyle(Style::FILL),
      mStrokeWidth(1.0f),
      mTextSize(12.0f),
      mAntiAlias(true) {
}

void Paint::setColor(uint32_t color) {
    mColor = color;
}

uint32_t Paint::getColor() const {
    return mColor;
}

void Paint::setStyle(Style style) {
    mStyle = style;
}

Style Paint::getStyle() const {
    return mStyle;
}

void Paint::setStrokeWidth(float width) {
    mStrokeWidth = width;
}

float Paint::getStrokeWidth() const {
    return mStrokeWidth;
}

void Paint::setTextSize(float textSize) {
    mTextSize = textSize;
}

float Paint::getTextSize() const {
    return mTextSize;
}

void Paint::setAntiAlias(bool aa) {
    mAntiAlias = aa;
}

bool Paint::isAntiAlias() const {
    return mAntiAlias;
}

} // namespace graphics
} // namespace windroid
