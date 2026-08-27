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

#include <algorithm>

namespace setu {
namespace graphics {

// Integer rectangle with the same semantics as android.graphics.Rect: left/top
// inclusive, right/bottom exclusive.
struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    Rect() = default;
    Rect(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}

    int width() const { return right - left; }
    int height() const { return bottom - top; }
    bool isEmpty() const { return right <= left || bottom <= top; }

    float exactCenterX() const { return (float)(left + right) * 0.5f; }
    float exactCenterY() const { return (float)(top + bottom) * 0.5f; }

    void set(int l, int t, int r, int b) { left = l; top = t; right = r; bottom = b; }
    void setEmpty() { left = top = right = bottom = 0; }

    void offset(int dx, int dy) { left += dx; top += dy; right += dx; bottom += dy; }
    void inset(int dx, int dy) { left += dx; top += dy; right -= dx; bottom -= dy; }

    bool contains(int x, int y) const {
        return left < right && top < bottom && x >= left && x < right && y >= top && y < bottom;
    }

    bool operator==(const Rect& other) const {
        return left == other.left && top == other.top &&
               right == other.right && bottom == other.bottom;
    }
    bool operator!=(const Rect& other) const { return !(*this == other); }
};

// Float rectangle, for geometry that has been inset by fractional stroke widths.
struct RectF {
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    RectF() = default;
    RectF(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    explicit RectF(const Rect& r)
        : left((float)r.left), top((float)r.top), right((float)r.right), bottom((float)r.bottom) {}

    float width() const { return right - left; }
    float height() const { return bottom - top; }
    bool isEmpty() const { return right <= left || bottom <= top; }

    float centerX() const { return (left + right) * 0.5f; }
    float centerY() const { return (top + bottom) * 0.5f; }

    void set(float l, float t, float r, float b) { left = l; top = t; right = r; bottom = b; }
    void inset(float dx, float dy) { left += dx; top += dy; right -= dx; bottom -= dy; }

    bool operator==(const RectF& other) const {
        return left == other.left && top == other.top &&
               right == other.right && bottom == other.bottom;
    }
    bool operator!=(const RectF& other) const { return !(*this == other); }
};

} // namespace graphics
} // namespace setu
