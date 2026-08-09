#pragma once
#include "Canvas.h"
#include "RenderNode.h"

namespace windroid {
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
    void drawText(const std::wstring& text, float x, float y, const Paint& paint) override;

    // Special command for drawing child RenderNodes
    void drawRenderNode(RenderNode* node);

private:
    RenderNode* mNode;
};

} // namespace graphics
} // namespace windroid
