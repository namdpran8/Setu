/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Paint.h"

namespace setu {
namespace graphics {

Paint::Paint()
    : mColor(0xFF000000), // Black, opaque
      mAlpha(1.0f),
      mStyle(Style::FILL),
      mStrokeWidth(1.0f),
      mTextSize(12.0f),
      mAntiAlias(true),
      mColorFilter(nullptr) {
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

void Paint::setShader(ShaderPtr shader) {
    mShader = shader;
}

ShaderPtr Paint::getShader() const {
    return mShader;
}

void Paint::setAlpha(float alpha) {
    mAlpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
}

float Paint::getAlpha() const {
    return mAlpha;
}

void Paint::setColorFilter(ColorFilterPtr filter) {
    mColorFilter = filter;
}

ColorFilterPtr Paint::getColorFilter() const {
    return mColorFilter;
}

} // namespace graphics
} // namespace setu
