#pragma once

#include "Paint.h"
#include <string>

namespace setu {
namespace graphics {

class RenderNode;

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

    virtual void drawRenderNode(RenderNode* node) = 0;
};

} // namespace graphics
} // namespace setu
