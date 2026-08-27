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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace setu {
namespace graphics {

// Mirrors android.graphics.Paint.FontMetrics, including its sign convention:
// everything above the baseline is negative, everything below is positive.
struct FontMetrics {
    float top = 0.0f;      // baseline -> top of the tallest glyph (negative)
    float ascent = 0.0f;   // baseline -> recommended top of a line (negative)
    float descent = 0.0f;  // baseline -> recommended bottom of a line (positive)
    float bottom = 0.0f;   // baseline -> bottom of the lowest glyph (positive)
    float leading = 0.0f;  // recommended additional space between lines

    float getLineHeight() const { return descent - ascent + leading; }
};

// Windroid's equivalent of android.text.StaticLayout: an immutable, already
// line-broken snapshot of one string measured with one Paint against one width.
//
// Its whole reason for existing is that measure and draw must not disagree.
// TextView builds one of these in onMeasure and walks the same line list in
// onDraw, so a view that measured two lines tall can never draw one clipped
// line the way it did when onDraw re-laid the text out on its own.
//
// Built exclusively by FontManager; there is no other way to produce one.
class TextLayout {
public:
    struct Line {
        uint32_t start = 0;     // index of the first character, into getText()
        uint32_t length = 0;    // visible characters: no line break, no trailing space
        float width = 0.0f;     // advance width of those visible characters
        float top = 0.0f;       // offset of the line box from the layout's top
        float height = 0.0f;    // line box height
        float baseline = 0.0f;  // offset of the baseline from the layout's top
    };

    TextLayout() = default;

    const std::wstring& getText() const { return mText; }

    int getLineCount() const { return (int)mLines.size(); }
    const Line& getLine(int index) const { return mLines[index]; }
    std::wstring getLineText(int index) const {
        const Line& line = mLines[index];
        return mText.substr(line.start, line.length);
    }

    // Widest line, and the sum of all line heights.
    float getWidth() const { return mWidth; }
    float getHeight() const { return mHeight; }

    const FontMetrics& getFontMetrics() const { return mFontMetrics; }

    // The inputs this layout was produced from, so callers can decide whether a
    // cached layout is still valid instead of rebuilding it every measure pass.
    float getTextSize() const { return mTextSize; }
    float getMaxWidth() const { return mMaxWidth; }
    bool matches(const std::wstring& text, float textSize, float maxWidth) const {
        return mTextSize == textSize && mMaxWidth == maxWidth && mText == text;
    }

private:
    friend class FontManager;

    std::wstring mText;
    std::vector<Line> mLines;
    FontMetrics mFontMetrics;
    float mWidth = 0.0f;
    float mHeight = 0.0f;
    float mTextSize = 0.0f;
    float mMaxWidth = 0.0f;
};

} // namespace graphics
} // namespace setu
