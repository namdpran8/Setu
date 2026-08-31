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

#include "Canvas.h"
#include <d2d1_1.h>
#include <memory>
#include "include/core/SkRefCnt.h"

class SkSurface;
class SkCanvas;
class SkPaint;

namespace setu {
namespace graphics {

class SkiaCanvas : public Canvas {
public:
    SkiaCanvas(int width, int height);
    ~SkiaCanvas() override;

    ::SkCanvas* getSkCanvas() override { return mCanvas; }

    void save() override;
    void restore() override;
    void translate(float dx, float dy) override;
    void scale(float sx, float sy) override;
    
    void clipRect(float left, float top, float right, float bottom) override;

    void drawColor(uint32_t color) override;
    void drawRect(float left, float top, float right, float bottom, const Paint& paint) override;
    void drawRoundRect(float left, float top, float right, float bottom, float rx, float ry, const Paint& paint) override;
    void drawLine(float startX, float startY, float stopX, float stopY, const Paint& paint) override;
    void drawText(const std::wstring& text, float x, float y, const Paint& paint) override;
    void drawPath(const Path& path, const Paint& paint) override;
    void drawBitmap(const Bitmap& bitmap, const RectF& src, const RectF& dst, const Paint& paint) override;
    void drawRenderNode(RenderNode* node) override;

    void blitToD2D(ID2D1DeviceContext* d2dContext);

private:
    void setupSkPaint(const Paint& paint, SkPaint* skPaint);

    sk_sp<SkSurface> mSurface;
    SkCanvas* mCanvas;
    int mWidth;
    int mHeight;
};

} // namespace graphics
} // namespace setu
