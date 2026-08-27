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

#include "Paint.h"
#include "Path.h"
#include "Rect.h"
#include <string>

namespace setu {
namespace graphics {

class RenderNode;

// Forward-declared, not included: drawBitmap only needs a reference, and Canvas.h
// is included by the whole view tree.
class Bitmap;

class Canvas {
public:
    virtual ~Canvas() = default;

    // State management
    virtual void save() = 0;
    virtual void restore() = 0;
    virtual void translate(float dx, float dy) = 0;
    virtual void scale(float sx, float sy) = 0;

    // Clipping
    virtual void clipRect(float left, float top, float right, float bottom) = 0;

    // Drawing
    virtual void drawColor(uint32_t color) = 0;
    virtual void drawRect(float left, float top, float right, float bottom, const Paint& paint) = 0;
    virtual void drawRoundRect(float left, float top, float right, float bottom, float rx, float ry, const Paint& paint) = 0;
    virtual void drawLine(float startX, float startY, float stopX, float stopY, const Paint& paint) = 0;
    virtual void drawText(const std::wstring& text, float x, float y, const Paint& paint) = 0;

    // Anything the axis-aligned primitives cannot express: per-corner rounded
    // rectangles, ovals, rings. Honours Paint's style and stroke width the same
    // way drawRect does.
    virtual void drawPath(const Path& path, const Paint& paint) = 0;

    // Canvas.drawBitmap(bitmap, srcRect, dstRect, paint). src selects a
    // sub-rectangle of the bitmap in *pixels*; dst is where it lands in the
    // canvas's own coordinates, and the two disagreeing is what scales the image.
    //
    // An empty src means the whole bitmap. Android spells that as a null Rect,
    // which a reference cannot express, so a default-constructed RectF stands in.
    //
    // Only the Paint's alpha is honoured. Its colour is ignored, exactly as in
    // Android, where tinting a bitmap needs a ColorFilter - and there is no
    // ColorFilter in this runtime yet.
    virtual void drawBitmap(const Bitmap& bitmap, const RectF& src, const RectF& dst,
                            const Paint& paint) = 0;

    virtual void drawRenderNode(RenderNode* node) = 0;
};

} // namespace graphics
} // namespace setu
