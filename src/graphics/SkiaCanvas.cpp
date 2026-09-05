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

#include "SkiaCanvas.h"
#include "RenderNode.h"
#include "Bitmap.h"
#include "../utils/Logger.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkColor.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkRect.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkFont.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkFontMgr.h"
#include "include/ports/SkTypeface_win.h"
#include "include/effects/SkGradientShader.h"
#include "include/core/SkString.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkImage.h"

#include <windows.h>
#include <wrl/client.h>
#include <map>
#include <tuple>
#include <string>

namespace setu {
namespace graphics {

SkiaCanvas::SkiaCanvas(int width, int height) : mWidth(width), mHeight(height) {
    SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);
    mSurface = SkSurfaces::Raster(info);
    if (mSurface) {
        mCanvas = mSurface->getCanvas();
    } else {
        Logger::e("SkiaCanvas", "Failed to create SkSurface");
        mCanvas = nullptr;
    }
}

SkiaCanvas::~SkiaCanvas() {
}

void SkiaCanvas::setupSkPaint(const Paint& paint, SkPaint* skPaint) {
    skPaint->setColor(paint.getColor());
    skPaint->setAlphaf(paint.getAlpha());
    skPaint->setAntiAlias(paint.isAntiAlias());
    
    if (paint.getStyle() == Style::STROKE) {
        skPaint->setStyle(SkPaint::kStroke_Style);
    } else if (paint.getStyle() == Style::FILL) {
        skPaint->setStyle(SkPaint::kFill_Style);
    } else {
        skPaint->setStyle(SkPaint::kStrokeAndFill_Style);
    }
    
    skPaint->setStrokeWidth(paint.getStrokeWidth());

    if (auto cf = paint.getColorFilter()) {
        if (cf->getType() == ColorFilterType::PORTER_DUFF) {
            auto pdcf = std::static_pointer_cast<PorterDuffColorFilter>(cf);
            SkBlendMode skBlendMode = SkBlendMode::kSrcIn;
            switch(pdcf->getMode()) {
                case BlendMode::SRC_IN: skBlendMode = SkBlendMode::kSrcIn; break;
                case BlendMode::SRC_ATOP: skBlendMode = SkBlendMode::kSrcATop; break;
                case BlendMode::SRC_OVER: skBlendMode = SkBlendMode::kSrcOver; break;
                case BlendMode::MULTIPLY: skBlendMode = SkBlendMode::kMultiply; break;
            }
            skPaint->setColorFilter(SkColorFilters::Blend(pdcf->getColor(), skBlendMode));
        }
    }

    if (auto shader = paint.getShader()) {
        auto toSkTileMode = [](TileMode mode) {
            switch (mode) {
                case TileMode::REPEAT: return SkTileMode::kRepeat;
                case TileMode::MIRROR: return SkTileMode::kMirror;
                default: return SkTileMode::kClamp;
            }
        };

        if (shader->getType() == ShaderType::LINEAR_GRADIENT) {
            auto linear = std::static_pointer_cast<LinearGradient>(shader);
            SkPoint pts[2] = {
                SkPoint::Make(linear->mX0, linear->mY0),
                SkPoint::Make(linear->mX1, linear->mY1)
            };
            skPaint->setShader(SkGradientShader::MakeLinear(
                pts, linear->mColors.data(),
                linear->mPositions.empty() ? nullptr : linear->mPositions.data(),
                linear->mColors.size(),
                toSkTileMode(linear->mTileMode)
            ));
        } else if (shader->getType() == ShaderType::RADIAL_GRADIENT) {
            auto radial = std::static_pointer_cast<RadialGradient>(shader);
            skPaint->setShader(SkGradientShader::MakeRadial(
                SkPoint::Make(radial->mCenterX, radial->mCenterY),
                radial->mRadius,
                radial->mColors.data(),
                radial->mPositions.empty() ? nullptr : radial->mPositions.data(),
                radial->mColors.size(),
                toSkTileMode(radial->mTileMode)
            ));
        } else if (shader->getType() == ShaderType::SWEEP_GRADIENT) {
            auto sweep = std::static_pointer_cast<SweepGradient>(shader);
            skPaint->setShader(SkGradientShader::MakeSweep(
                sweep->mCenterX, sweep->mCenterY,
                sweep->mColors.data(),
                sweep->mPositions.empty() ? nullptr : sweep->mPositions.data(),
                sweep->mColors.size()
            ));
        }
    }
}

void SkiaCanvas::save() {
    if (mCanvas) mCanvas->save();
}

void SkiaCanvas::restore() {
    if (mCanvas) mCanvas->restore();
}

void SkiaCanvas::translate(float dx, float dy) {
    if (mCanvas) mCanvas->translate(dx, dy);
}

void SkiaCanvas::scale(float sx, float sy) {
    if (mCanvas) mCanvas->scale(sx, sy);
}

void SkiaCanvas::clipRect(float left, float top, float right, float bottom) {
    if (mCanvas) {
        SkRect rect = SkRect::MakeLTRB(left, top, right, bottom);
        mCanvas->clipRect(rect, true);
    }
}

void SkiaCanvas::drawColor(uint32_t color) {
    if (mCanvas) mCanvas->clear(color);
}

void SkiaCanvas::drawRect(float left, float top, float right, float bottom, const Paint& paint) {
    if (!mCanvas) return;
    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);
    mCanvas->drawRect(SkRect::MakeLTRB(left, top, right, bottom), skPaint);
}

void SkiaCanvas::drawRoundRect(float left, float top, float right, float bottom, float rx, float ry, const Paint& paint) {
    if (!mCanvas) return;
    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);
    mCanvas->drawRoundRect(SkRect::MakeLTRB(left, top, right, bottom), rx, ry, skPaint);
}

void SkiaCanvas::drawLine(float startX, float startY, float stopX, float stopY, const Paint& paint) {
    if (!mCanvas) return;
    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);
    mCanvas->drawLine(startX, startY, stopX, stopY, skPaint);
}

void SkiaCanvas::drawText(const std::wstring& text, float x, float y, const Paint& paint) {
    if (!mCanvas) return;
    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);
    
    struct FontStyleCompare {
        bool operator()(const std::pair<std::string, SkFontStyle>& a, const std::pair<std::string, SkFontStyle>& b) const {
            if (a.first != b.first) return a.first < b.first;
            if (a.second.weight() != b.second.weight()) return a.second.weight() < b.second.weight();
            if (a.second.width() != b.second.width()) return a.second.width() < b.second.width();
            return a.second.slant() < b.second.slant();
        }
    };
    
    static std::map<std::pair<std::string, SkFontStyle>, sk_sp<SkTypeface>, FontStyleCompare> typefaceCache;
    static sk_sp<SkFontMgr> fontMgr = SkFontMgr_New_DirectWrite();
    
    std::string familyName = "Segoe UI";
    SkFontStyle fontStyle = SkFontStyle::Normal();
    auto key = std::make_pair(familyName, fontStyle);
    
    sk_sp<SkTypeface> typeface;
    auto it = typefaceCache.find(key);
    if (it != typefaceCache.end()) {
        typeface = it->second;
    } else {
        typeface = fontMgr ? fontMgr->matchFamilyStyle(familyName.c_str(), fontStyle) : nullptr;
        typefaceCache[key] = typeface;
    }
    
    SkFont font(typeface, paint.getTextSize());
    
    // Convert wstring to string (UTF-8) for Skia
    std::string utf8;
    int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size > 0) {
        utf8.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8[0], size, nullptr, nullptr);
    }
    
    mCanvas->drawString(utf8.c_str(), x, y, font, skPaint);
}

void SkiaCanvas::drawPath(const Path& path, const Paint& paint) {
    if (!mCanvas) return;
    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);
    
    SkPath skPath;
    if (path.getFillType() == Path::FillType::EVEN_ODD) {
        skPath.setFillType(SkPathFillType::kEvenOdd);
    } else {
        skPath.setFillType(SkPathFillType::kWinding);
    }
// If you found this, you are probably debugging this at an
// unreasonable hour.
//
// Go drink some water.
// The bug can wait.
//
// — Previous developer


    const auto& verbs = path.getVerbs();
    const auto& points = path.getPoints();
    size_t pointIdx = 0;

    for (auto verb : verbs) {
        switch (verb) {
            case Path::Verb::MOVE_TO:
                skPath.moveTo(points[pointIdx], points[pointIdx + 1]);
                pointIdx += 2;
                break;
            case Path::Verb::LINE_TO:
                skPath.lineTo(points[pointIdx], points[pointIdx + 1]);
                pointIdx += 2;
                break;
            case Path::Verb::CUBIC_TO:
                skPath.cubicTo(points[pointIdx], points[pointIdx + 1],
                               points[pointIdx + 2], points[pointIdx + 3],
                               points[pointIdx + 4], points[pointIdx + 5]);
                pointIdx += 6;
                break;
            case Path::Verb::CLOSE:
                skPath.close();
                break;
        }
    }

    mCanvas->drawPath(skPath, skPaint);
}

void SkiaCanvas::drawBitmap(const Bitmap& bitmap, const RectF& src, const RectF& dst, const Paint& paint) {
    if (!mCanvas) return;
    
    // Convert setu::graphics::Bitmap to SkImage
    SkImageInfo info = SkImageInfo::MakeN32Premul(bitmap.getWidth(), bitmap.getHeight());
    SkPixmap pixmap(info, bitmap.getPixels(), bitmap.getRowBytes());
    sk_sp<SkImage> image = SkImages::RasterFromPixmapCopy(pixmap);
    
    if (!image) return;

    SkPaint skPaint;
    setupSkPaint(paint, &skPaint);

    SkRect skSrc;
    if (src.isEmpty()) {
        skSrc = SkRect::MakeLTRB(0.0f, 0.0f, (float)bitmap.getWidth(), (float)bitmap.getHeight());
    } else {
        skSrc = SkRect::MakeLTRB(src.left, src.top, src.right, src.bottom);
    }
    
    SkRect skDst = SkRect::MakeLTRB(dst.left, dst.top, dst.right, dst.bottom);

    mCanvas->drawImageRect(image, skSrc, skDst, SkSamplingOptions(SkFilterMode::kLinear), &skPaint, SkCanvas::kStrict_SrcRectConstraint);
}

void SkiaCanvas::drawRenderNode(RenderNode* node) {
    if (node && mCanvas) {
        node->draw(*this);
    }
}

void SkiaCanvas::blitToD2D(ID2D1DeviceContext* d2dContext) {
    if (!mSurface || !d2dContext) return;

    SkPixmap pixmap;
    if (mSurface->peekPixels(&pixmap)) {
        D2D1_BITMAP_PROPERTIES properties = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );

        Microsoft::WRL::ComPtr<ID2D1Bitmap> d2dBitmap;
        HRESULT hr = d2dContext->CreateBitmap(
            D2D1::SizeU(pixmap.width(), pixmap.height()),
            pixmap.addr(),
            pixmap.rowBytes(),
            properties,
            &d2dBitmap
        );

        if (SUCCEEDED(hr)) {
            d2dContext->DrawBitmap(
                d2dBitmap.Get(),
                D2D1::RectF(0, 0, (float)mWidth, (float)mHeight),
                1.0f,
                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
                nullptr
            );
        } else {
            Logger::e("SkiaCanvas", "Failed to create D2D bitmap for blit");
        }
    }
}

} // namespace graphics
} // namespace setu
