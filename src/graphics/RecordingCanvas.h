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
#include "RenderNode.h"

namespace setu {
namespace graphics {

// A canvas that records operations into a RenderNode instead of drawing immediately
class RecordingCanvas : public Canvas {
public:
    RecordingCanvas(RenderNode* node);
    ~RecordingCanvas() override = default;

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

    // Special command for drawing child RenderNodes
    void drawRenderNode(RenderNode* node);

private:
    RenderNode* mNode;
};

} // namespace graphics
} // namespace setu
