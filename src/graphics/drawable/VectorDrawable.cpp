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

#include "VectorDrawable.h"
#include "../Canvas.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathMeasure.h"
#include "include/core/SkMatrix.h"

namespace setu {
namespace graphics {

void VectorDrawable::draw(Canvas& canvas) {
    SkCanvas* skCanvas = canvas.getSkCanvas();
    if (!skCanvas) {
        // Direct2D fallback (or any non-Skia canvas)
        return;
    }

    if (mViewportWidth <= 0.0f || mViewportHeight <= 0.0f) {
        return;
    }

    skCanvas->save();
    
    // 1. Map drawable bounds
    const Rect& bounds = getBounds();
    skCanvas->translate((float)bounds.left + mTranslateX, (float)bounds.top + mTranslateY);
    skCanvas->scale(mScaleX, mScaleY);

    // 2. Draw root group recursively
    if (mRootGroup) drawGroup(skCanvas, *mRootGroup);

    skCanvas->restore();
}

void VectorDrawable::drawGroup(SkCanvas* canvas, const VGroup& group) {
    canvas->save();
    
    // Apply group transforms (Android order: translate -> rotate -> scale around pivot)
    canvas->translate(group.translateX + group.pivotX, group.translateY + group.pivotY);
    canvas->rotate(group.rotation);
    canvas->scale(group.scaleX, group.scaleY);
    canvas->translate(-group.pivotX, -group.pivotY);

    for (const auto& childGroup : group.groups) {
        drawGroup(canvas, *childGroup);
    }
    for (const auto& path : group.paths) {
        drawPath(canvas, *path);
    }

    canvas->restore();
}

void VectorDrawable::drawPath(SkCanvas* canvas, const VPath& vpath) {
    SkPath pathToDraw = vpath.path;
    pathToDraw.setFillType(vpath.fillEvenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding);

    // Trim path logic
    if (vpath.trimPathStart > 0.0f || vpath.trimPathEnd < 1.0f) {
        SkPathMeasure measure(vpath.path, false);
        float length = measure.getLength();
        float start = (vpath.trimPathStart + vpath.trimPathOffset) * length;
        float end = (vpath.trimPathEnd + vpath.trimPathOffset) * length;
        
        // Wrap around logic if start > end or offset pushes it past 1.0
        // (Simplified for now - handles standard 0.0 to 1.0 with no offset wrapping)
        // A full implementation would handle fmod(start, length).
        
        SkPath trimmed;
        if (start > end) {
            measure.getSegment(start, length, &trimmed, true);
            measure.getSegment(0, end, &trimmed, true);
        } else {
            measure.getSegment(start, end, &trimmed, true);
        }
        pathToDraw = trimmed;
    }

    // Apply alpha from the Drawable
    float alphaScale = mAlpha / 255.0f;

    if (vpath.hasFill()) {
        SkPaint paint;
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);
        
        float a = ((vpath.fillColor >> 24) & 0xFF) / 255.0f * vpath.fillAlpha * alphaScale;
        float r = ((vpath.fillColor >> 16) & 0xFF) / 255.0f;
        float g = ((vpath.fillColor >> 8) & 0xFF) / 255.0f;
        float b = (vpath.fillColor & 0xFF) / 255.0f;
        paint.setColor4f({r, g, b, a});
        
        canvas->drawPath(pathToDraw, paint);
    }

    if (vpath.hasStroke()) {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setAntiAlias(true);
        paint.setStrokeWidth(vpath.strokeWidth);
        
        switch (vpath.strokeLineCap) {
            case VPath::LineCap::ROUND: paint.setStrokeCap(SkPaint::kRound_Cap); break;
            case VPath::LineCap::SQUARE: paint.setStrokeCap(SkPaint::kSquare_Cap); break;
            default: paint.setStrokeCap(SkPaint::kButt_Cap); break;
        }
        
        switch (vpath.strokeLineJoin) {
            case VPath::LineJoin::ROUND: paint.setStrokeJoin(SkPaint::kRound_Join); break;
            case VPath::LineJoin::BEVEL: paint.setStrokeJoin(SkPaint::kBevel_Join); break;
            default: paint.setStrokeJoin(SkPaint::kMiter_Join); break;
        }

        float a = ((vpath.strokeColor >> 24) & 0xFF) / 255.0f * vpath.strokeAlpha * alphaScale;
        float r = ((vpath.strokeColor >> 16) & 0xFF) / 255.0f;
        float g = ((vpath.strokeColor >> 8) & 0xFF) / 255.0f;
        float b = (vpath.strokeColor & 0xFF) / 255.0f;
        paint.setColor4f({r, g, b, a});

        canvas->drawPath(pathToDraw, paint);
    }
}

void VectorDrawable::setAlpha(int alpha) {
    if (mAlpha != alpha) {
        mAlpha = alpha;
        invalidateSelf();
    }
}

void VectorDrawable::onBoundsChange(const Rect& bounds) {
    if (mViewportWidth > 0 && mViewportHeight > 0) {
        float boundsW = (float)bounds.width();
        float boundsH = (float)bounds.height();
        
        mScaleX = boundsW / mViewportWidth;
        mScaleY = boundsH / mViewportHeight;
        mTranslateX = 0.0f;
        mTranslateY = 0.0f;
    }
}

} // namespace graphics
} // namespace setu
