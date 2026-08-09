#include "Direct2DCanvas.h"

namespace windroid {
namespace graphics {

Direct2DCanvas::Direct2DCanvas(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory)
    : mContext(context), mDWriteFactory(dwriteFactory) {
    State initialState;
    mContext->GetTransform(&initialState.transform);
    mStateStack.push_back(initialState);
}

void Direct2DCanvas::save() {
    State s;
    mContext->GetTransform(&s.transform);
    mStateStack.push_back(s);
}

void Direct2DCanvas::restore() {
    if (mStateStack.size() > 1) {
        mStateStack.pop_back();
        mContext->SetTransform(mStateStack.back().transform);
    }
}

void Direct2DCanvas::translate(float dx, float dy) {
    D2D1_MATRIX_3X2_F current;
    mContext->GetTransform(&current);
    mContext->SetTransform(current * D2D1::Matrix3x2F::Translation(dx, dy));
}

void Direct2DCanvas::scale(float sx, float sy) {
    D2D1_MATRIX_3X2_F current;
    mContext->GetTransform(&current);
    mContext->SetTransform(current * D2D1::Matrix3x2F::Scale(sx, sy));
}

void Direct2DCanvas::clipRect(float left, float top, float right, float bottom) {
    mContext->PushAxisAlignedClip(D2D1::RectF(left, top, right, bottom), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    // Note: Android Canvas clipRect is a persistent state until restore().
    // Direct2D PushAxisAlignedClip requires a matching PopAxisAlignedClip.
    // For a full implementation, we'd need to track clip layers in the State stack and pop them on restore.
    // We will keep it simple for milestone 1.
}

ID2D1SolidColorBrush* Direct2DCanvas::getCachedBrush(uint32_t color) {
    if (!mSolidBrush || mLastBrushColor != color) {
        mSolidBrush.Reset();
        float a = ((color >> 24) & 0xFF) / 255.0f;
        float r = ((color >> 16) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = (color & 0xFF) / 255.0f;
        mContext->CreateSolidColorBrush(D2D1::ColorF(r, g, b, a), &mSolidBrush);
        mLastBrushColor = color;
    }
    return mSolidBrush.Get();
}

void Direct2DCanvas::drawColor(uint32_t color) {
    mContext->Clear(D2D1::ColorF(
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f,
        ((color >> 24) & 0xFF) / 255.0f
    ));
}

void Direct2DCanvas::drawRect(float left, float top, float right, float bottom, const Paint& paint) {
    auto brush = getCachedBrush(paint.getColor());
    if (paint.getStyle() == Style::FILL || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->FillRectangle(D2D1::RectF(left, top, right, bottom), brush);
    }
    if (paint.getStyle() == Style::STROKE || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->DrawRectangle(D2D1::RectF(left, top, right, bottom), brush, paint.getStrokeWidth());
    }
}

void Direct2DCanvas::drawRoundRect(float left, float top, float right, float bottom, float rx, float ry, const Paint& paint) {
    auto brush = getCachedBrush(paint.getColor());
    D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), rx, ry);
    
    if (paint.getStyle() == Style::FILL || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->FillRoundedRectangle(rrect, brush);
    }
    if (paint.getStyle() == Style::STROKE || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->DrawRoundedRectangle(rrect, brush, paint.getStrokeWidth());
    }
}

void Direct2DCanvas::drawText(const std::wstring& text, float x, float y, const Paint& paint) {
    if (!mDWriteFactory || text.empty()) return;

    Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
    mDWriteFactory->CreateTextFormat(
        L"Segoe UI",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        paint.getTextSize(),
        L"en-us",
        &textFormat
    );

    if (textFormat) {
        Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
        mDWriteFactory->CreateTextLayout(
            text.c_str(),
            (UINT32)text.length(),
            textFormat.Get(),
            10000.0f, // Max width
            10000.0f, // Max height
            &textLayout
        );

        if (textLayout) {
            auto brush = getCachedBrush(paint.getColor());
            
            // Android draws text from the baseline. Direct2D draws from top-left.
            // We need to query the metrics to offset it properly.
            DWRITE_TEXT_METRICS metrics;
            textLayout->GetMetrics(&metrics);
            
            // Note: We need a more accurate baseline calculation in Phase 3.
            // For now, we'll approximate the baseline correction.
            mContext->DrawTextLayout(D2D1::Point2F(x, y - paint.getTextSize()), textLayout.Get(), brush);
        }
    }
}

void Direct2DCanvas::drawRenderNode(RenderNode* node) {
    if (node) {
        node->draw(*this);
    }
}

} // namespace graphics
} // namespace windroid
