#pragma once

#include "Canvas.h"
#include "RenderNode.h"
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl/client.h>
#include <vector>

namespace setu {
namespace graphics {

class Direct2DCanvas : public Canvas {
public:
    Direct2DCanvas(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory);
    ~Direct2DCanvas() override = default;

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
    void drawRenderNode(RenderNode* node) override;

    // Helper to get or create a solid color brush based on Paint
    ID2D1SolidColorBrush* getCachedBrush(uint32_t color);

private:
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> mContext;
    Microsoft::WRL::ComPtr<IDWriteFactory> mDWriteFactory;
    
    // Brush cache for performance
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mSolidBrush;
    uint32_t mLastBrushColor = 0;

    struct State {
        D2D1_MATRIX_3X2_F transform;
    };
    std::vector<State> mStateStack;
};

} // namespace graphics
} // namespace setu
