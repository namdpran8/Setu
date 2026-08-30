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

#include "Direct2DCanvas.h"
#include <algorithm>
#include <unordered_map>
#include "Bitmap.h"
#include "FontManager.h"
#include "../utils/Logger.h"
namespace setu {
namespace graphics {

Direct2DCanvas::Direct2DCanvas(ID2D1DeviceContext* context, IDWriteFactory* dwriteFactory)
    : mContext(context), mDWriteFactory(dwriteFactory) {
    // Keep FontManager pointed at the factory we actually render with, so the
    // format a view measured itself with is the format its text draws with.
    // This is a no-op after the first frame.
    FontManager::getInstance().setFactory(dwriteFactory);

    State initialState;
    mContext->GetTransform(&initialState.transform);
    initialState.clipCount = 0;
    mStateStack.push_back(initialState);
}

/*
I know what I did
I know what I said
But that doesn’t mean I don’t care
And maybe you’re right I’ll have my regrets
But right now with you standing there
What’s killing me the most
Is how you look at me different
I didn’t want to go
But it don’t make a difference
Now you been saying you don’t believe inBelieve in love without leaving 
And what’s killing me the mostIs I know that I’m the reason
:- The Reason (Acoustic) - HARIZ 
*/

void Direct2DCanvas::save() {
    State s;
    mContext->GetTransform(&s.transform);
    s.clipCount = 0;
    mStateStack.push_back(s);
}

// If you\'re reading this, you\'re probably trying to understand what this does.
// Good luck. The comment above was written by AI because I didn\'t want to
// explain it myself. I hope it was correct.
void Direct2DCanvas::restore() {
    if (mStateStack.size() > 1) {
        State s = mStateStack.back();
        mStateStack.pop_back();
        mContext->SetTransform(s.transform);
        for (int i = 0; i < s.clipCount; i++) {
            mContext->PopAxisAlignedClip();
        }
    }
}

void Direct2DCanvas::translate(float dx, float dy) {
    D2D1_MATRIX_3X2_F current;
    mContext->GetTransform(&current);
    mContext->SetTransform(D2D1::Matrix3x2F::Translation(dx, dy) * current);
}

void Direct2DCanvas::scale(float sx, float sy) {
    D2D1_MATRIX_3X2_F current;
    mContext->GetTransform(&current);
    mContext->SetTransform(D2D1::Matrix3x2F::Scale(sx, sy) * current);
}

void Direct2DCanvas::clipRect(float left, float top, float right, float bottom) {
    mContext->PushAxisAlignedClip(D2D1::RectF(left, top, right, bottom), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    if (!mStateStack.empty()) {
        mStateStack.back().clipCount++;
    }
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

void Direct2DCanvas::drawLine(float startX, float startY, float stopX, float stopY, const Paint& paint) {
    if (!mContext) return;
    
    auto brush = getCachedBrush(paint.getColor());
    if (!brush) return;
    
    D2D1_POINT_2F start = D2D1::Point2F(startX, startY);
    D2D1_POINT_2F stop = D2D1::Point2F(stopX, stopY);
    
    mContext->DrawLine(start, stop, brush, paint.getStrokeWidth());
}

void Direct2DCanvas::drawText(const std::wstring& text, float x, float y, const Paint& paint) {
    if (!mDWriteFactory || text.empty()) return;

    FontManager& fonts = FontManager::getInstance();
    IDWriteTextFormat* textFormat = fonts.getTextFormat(paint);
    if (!textFormat) return;

    // Text arriving here is always a single, already-broken line: TextView does
    // its own line breaking through FontManager so that what it measured is what
    // gets drawn. Laying out at an unbounded width guarantees we cannot silently
    // re-break it into a different set of lines.
    Microsoft::WRL::ComPtr<IDWriteTextLayout> textLayout;
    HRESULT hr = mDWriteFactory->CreateTextLayout(
        text.c_str(), (UINT32)text.length(), textFormat,
        FontManager::UNBOUNDED_WIDTH, FontManager::UNBOUNDED_WIDTH, &textLayout);

    if (FAILED(hr) || !textLayout) {
        Logger::e("Direct2DCanvas", "CreateTextLayout failed: 0x" + std::to_string(hr));
        return;
    }

    auto brush = getCachedBrush(paint.getColor());
    if (!brush) {
        Logger::e("Direct2DCanvas", "Failed to create/get brush for color 0x" + std::to_string(paint.getColor()));
        return;
    }

    // Android's drawText() y is the baseline; DrawTextLayout's origin is the top
    // of the line box, so shift up by the ascent.
    const float baseline = -fonts.getFontMetrics(paint).ascent;
    mContext->DrawTextLayout(D2D1::Point2F(x, y - baseline), textLayout.Get(), brush);
}

void Direct2DCanvas::drawPath(const Path& path, const Paint& paint) {
    if (!mContext || path.isEmpty()) return;

    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
    mContext->GetFactory(&factory);
    if (!factory) return;

    Microsoft::WRL::ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(factory->CreatePathGeometry(&geometry)) || !geometry) return;

    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(&sink)) || !sink) return;

    // Must be set before the first figure, otherwise D2D ignores it - which is
    // how a ring silently comes out as a filled disc.
    sink->SetFillMode(path.getFillType() == Path::FillType::EVEN_ODD
                          ? D2D1_FILL_MODE_ALTERNATE
                          : D2D1_FILL_MODE_WINDING);

    const std::vector<Path::Verb>& verbs = path.getVerbs();
    const std::vector<float>& pts = path.getPoints();
    size_t p = 0;
    bool figureOpen = false;

    auto take = [&pts, &p]() -> D2D1_POINT_2F {
        const D2D1_POINT_2F pt = D2D1::Point2F(pts[p], pts[p + 1]);
        p += 2;
        return pt;
    };

    for (Path::Verb verb : verbs) {
        switch (verb) {
            case Path::Verb::MOVE_TO: {
                if (p + 2 > pts.size()) break;
                if (figureOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
                sink->BeginFigure(take(), D2D1_FIGURE_BEGIN_FILLED);
                figureOpen = true;
                break;
            }
            case Path::Verb::LINE_TO: {
                if (!figureOpen || p + 2 > pts.size()) break;
                sink->AddLine(take());
                break;
            }
            case Path::Verb::CUBIC_TO: {
                if (!figureOpen || p + 6 > pts.size()) break;
                D2D1_BEZIER_SEGMENT seg;
                seg.point1 = take();
                seg.point2 = take();
                seg.point3 = take();
                sink->AddBezier(seg);
                break;
            }
            case Path::Verb::CLOSE: {
                if (!figureOpen) break;
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                figureOpen = false;
                break;
            }
        }
    }
    if (figureOpen) sink->EndFigure(D2D1_FIGURE_END_OPEN);
    if (FAILED(sink->Close())) return;

    auto brush = getCachedBrush(paint.getColor());
    if (!brush) return;

    if (paint.getStyle() == Style::FILL || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->FillGeometry(geometry.Get(), brush);
    }
    if (paint.getStyle() == Style::STROKE || paint.getStyle() == Style::FILL_AND_STROKE) {
        mContext->DrawGeometry(geometry.Get(), brush, paint.getStrokeWidth());
    }
}

void Direct2DCanvas::drawBitmap(const Bitmap& bitmap, const RectF& src, const RectF& dst,
                                const Paint& paint) {
    if (!mContext || dst.isEmpty()) return;

    // getD2DBitmap is const: the upload it may do is a cache fill, not a change to
    // the image.
    ID2D1Bitmap* device = bitmap.getD2DBitmap(mContext.Get());
    if (!device) return; // Bitmap has already logged why

    // The device bitmap is created at 96 DPI (see Bitmap::getD2DBitmap), so one
    // source DIP is one bitmap pixel and src needs no conversion.
    const float w = (float)bitmap.getWidth();
    const float h = (float)bitmap.getHeight();

    // Empty src means the whole bitmap - Android's null Rect. Clamped in either
    // case, because D2D fails the whole draw on a source rectangle that leaves the
    // bitmap where Skia would have clamped to the edge.
    D2D1_RECT_F srcRect =
        src.isEmpty() ? D2D1::RectF(0.0f, 0.0f, w, h)
                      : D2D1::RectF(std::max(0.0f, src.left), std::max(0.0f, src.top),
                                    std::min(w, src.right), std::min(h, src.bottom));
    if (srcRect.right <= srcRect.left || srcRect.bottom <= srcRect.top) return;

    const D2D1_RECT_F dstRect = D2D1::RectF(dst.left, dst.top, dst.right, dst.bottom);

    // Alpha only, per the contract on Canvas::drawBitmap. Fully transparent is a
    // real case - a fading drawable - and skipping it saves the sampler.
    const float opacity = (float)((paint.getColor() >> 24) & 0xFF) / 255.0f;
    if (opacity <= 0.0f) return;

    // Pointer arguments deliberately, to pin overload resolution to
    // ID2D1DeviceContext::DrawBitmap. The ID2D1RenderTarget::DrawBitmap this also
    // inherits takes a D2D1_BITMAP_INTERPOLATION_MODE instead, a different enum
    // that has no LINEAR value of its own to confuse this with.
    mContext->DrawBitmap(device, &dstRect, opacity, D2D1_INTERPOLATION_MODE_LINEAR, &srcRect,
                         nullptr);
}

void Direct2DCanvas::drawRenderNode(RenderNode* node) {
    if (node) {
        node->draw(*this);
    }
}

} // namespace graphics
} // namespace setu
