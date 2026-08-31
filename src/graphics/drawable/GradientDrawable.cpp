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

#include "GradientDrawable.h"

#include <algorithm>

#include "../Canvas.h"

namespace setu {
namespace graphics {

void GradientDrawable::setShape(Shape shape) {
    if (mShape == shape) return;
    mShape = shape;
    mPathDirty = true;
    invalidateSelf();
}

void GradientDrawable::setColor(uint32_t argb) {
    if (mHasSolid && !mSolidCsl && mSolidColor == argb) return;
    // An explicit flat colour replaces any selector that was there.
    mSolidCsl.reset();
    mHasSolid = true;
    mSolidColor = argb;
    invalidateSelf();
}

void GradientDrawable::clearColor() {
    if (!mHasSolid && !mSolidCsl) return;
    mSolidCsl.reset();
    mHasSolid = false;
    mSolidColor = 0x00000000;
    invalidateSelf();
}

void GradientDrawable::setColor(const ColorStateListPtr& csl) {
    if (!csl) {
        clearColor();
        return;
    }
    mSolidCsl = csl;
    mHasSolid = true;
    // Resolve against the state already held. A drawable is often handed its
    // bounds and state before its colours, and AOSP's setColor(ColorStateList)
    // resolves immediately for the same reason - otherwise the shape paints its
    // default colour until the next unrelated state change happens to arrive.
    mSolidColor = csl->getColorForState(getState(), 0x00000000);
    invalidateSelf();
}

void GradientDrawable::setStrokeColorStateList(const ColorStateListPtr& csl) {
    mStrokeCsl = csl;
    if (csl) {
        mStrokeColor = csl->getColorForState(getState(), 0x00000000);
    }
    // The stroke's alpha decides whether a stroke exists at all, and that decides
    // the fill inset - so the geometry has to be recomputed, not just repainted.
    ensureValidRect();
    mPathDirty = true;
    invalidateSelf();
}

bool GradientDrawable::isStateful() const {
    return (mSolidCsl && mSolidCsl->isStateful()) ||
           (mStrokeCsl && mStrokeCsl->isStateful());
}

bool GradientDrawable::onStateChange(const std::vector<int>& stateSet) {
    bool changed = false;

    if (mSolidCsl) {
        // AOSP passes 0 as the fallback: an unmatched state means no fill, not
        // some arbitrary colour. A selector that wants a default supplies a
        // wildcard item, and every real one does.
        const uint32_t color = mSolidCsl->getColorForState(stateSet, 0x00000000);
        if (mSolidColor != color) {
            mSolidColor = color;
            changed = true;
        }
    }

    if (mStrokeCsl) {
        const uint32_t color = mStrokeCsl->getColorForState(stateSet, 0x00000000);
        if (mStrokeColor != color) {
            mStrokeColor = color;
            // A stroke that becomes transparent stops existing as far as
            // hasStroke() is concerned, which moves the fill's edge. Recompute
            // rather than let the old inset survive into the new state.
            ensureValidRect();
            mPathDirty = true;
            changed = true;
        }
    }

    return changed;
}

void GradientDrawable::setCornerRadius(float radius) {
    if (radius < 0.0f) radius = 0.0f;
    if (!mHasRadiusArray && mRadius == radius) return;
    mRadius = radius;
    mHasRadiusArray = false;
    mPathDirty = true;
    invalidateSelf();
}

void GradientDrawable::setCornerRadii(const float radii[8]) {
    if (!radii) {
        if (!mHasRadiusArray) return;
        mHasRadiusArray = false;
        mPathDirty = true;
        invalidateSelf();
        return;
    }
    for (int i = 0; i < 8; ++i) {
        mRadiusArray[i] = radii[i] < 0.0f ? 0.0f : radii[i];
    }
    mHasRadiusArray = true;
    mPathDirty = true;
    invalidateSelf();
}

void GradientDrawable::setStroke(float width, uint32_t color) {
    setStroke(width, color, 0.0f, 0.0f);
}

void GradientDrawable::setStroke(float width, uint32_t color, float dashWidth, float dashGap) {
    mStrokeWidth = width;
    // An explicit flat colour replaces any selector that was there.
    mStrokeCsl.reset();
    mStrokeColor = color;
    mStrokeDashWidth = dashWidth;
    mStrokeDashGap = dashGap;
    // The stroke changes where the fill ends, not just what sits on top of it.
    ensureValidRect();
    mPathDirty = true;
    invalidateSelf();
}

void GradientDrawable::setRingGeometry(float innerRadius, float innerRadiusRatio,
                                      float thickness, float thicknessRatio) {
    mInnerRadius = innerRadius;
    mInnerRadiusRatio = innerRadiusRatio;
    mThickness = thickness;
    mThicknessRatio = thicknessRatio;
    mPathDirty = true;
    invalidateSelf();
}

void GradientDrawable::buildRingPath() {
    mPath.reset();

    // AOSP's buildRing, full sweep only: android:useLevel is not supported, so the
    // ring is always the complete annulus rather than a partial arc.
    const float width = mRect.width();
    const float halfW = width * 0.5f;
    const float halfH = mRect.height() * 0.5f;

    // Both of these divide the *width*, including the one that ends up driving a
    // vertical inset. That is AOSP's arithmetic rather than a slip here, and it is
    // what makes a ring in a non-square view match a real device.
    const float thickness = mThickness != -1.0f
                                ? mThickness
                                : (mThicknessRatio != 0.0f ? width / mThicknessRatio : 0.0f);
    const float radius = mInnerRadius != -1.0f
                             ? mInnerRadius
                             : (mInnerRadiusRatio != 0.0f ? width / mInnerRadiusRatio : 0.0f);

    // The hole: centred in the bounds with a half-extent of `radius` on both axes.
    RectF inner = mRect;
    inner.inset(halfW - radius, halfH - radius);
    // The outer edge, grown from the hole by the ring's thickness.
    RectF outer = inner;
    outer.inset(-thickness, -thickness);

    // Path::addOval always winds clockwise, so the hole has to come from the fill
    // rule: EVEN_ODD leaves the inner oval unpainted. AOSP instead winds the two
    // in opposite directions under the default rule, which for two nested ovals
    // produces the same band.
    //
    // If the inner oval collapses (radius 0, or a degenerate rect), addOval adds
    // nothing and this paints a solid disc - the same graceful fallback AOSP gets
    // from a zero-radius ring.
    mPath.setFillType(Path::FillType::EVEN_ODD);
    mPath.addOval(outer);
    mPath.addOval(inner);
}

void GradientDrawable::setPaddingInsets(int left, int top, int right, int bottom) {
    mPaddingInsets.set(left, top, right, bottom);
    mHasPadding = true;
}

bool GradientDrawable::getPadding(Rect& padding) const {
    if (!mHasPadding) {
        padding.setEmpty();
        return false;
    }
    padding = mPaddingInsets;
    return true;
}

void GradientDrawable::setSize(int width, int height) {
    mWidth = width;
    mHeight = height;
}

void GradientDrawable::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mAlpha == alpha) return;
    mAlpha = alpha;
    invalidateSelf();
}

uint32_t GradientDrawable::applyAlpha(uint32_t argb) const {
    if (mAlpha >= 255) return argb;
    const uint32_t baseAlpha = (argb >> 24) & 0xFF;
    const uint32_t scaled = (baseAlpha * (uint32_t)mAlpha) / 255u;
    return (scaled << 24) | (argb & 0x00FFFFFF);
}

void GradientDrawable::setGradient(GradientType type, float angle, float centerX, float centerY, float gradientRadius,
                                   uint32_t startColor, uint32_t centerColor, uint32_t endColor, bool hasCenterColor,
                                   TileMode tileMode) {
    mHasGradient = true;
    mGradientState.type = type;
    mGradientState.angle = angle;
    mGradientState.centerX = centerX;
    mGradientState.centerY = centerY;
    mGradientState.gradientRadius = gradientRadius;
    mGradientState.startColor = startColor;
    mGradientState.centerColor = centerColor;
    mGradientState.endColor = endColor;
    mGradientState.hasCenterColor = hasCenterColor;
    mGradientState.tileMode = tileMode;
    
    updateShader();
    invalidateSelf();
}

#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void GradientDrawable::updateShader() {
    if (!mHasGradient || mRect.isEmpty()) {
        mShader.reset();
        return;
    }

    std::vector<uint32_t> colors;
    if (mGradientState.hasCenterColor) {
        colors = {mGradientState.startColor, mGradientState.centerColor, mGradientState.endColor};
    } else {
        colors = {mGradientState.startColor, mGradientState.endColor};
    }
    std::vector<float> positions; // Empty implies even distribution

    float w = mRect.width();
    float h = mRect.height();

    if (mGradientState.type == GradientType::LINEAR) {
        // Android maps angle to a line crossing the bounds.
        // angle is in degrees, counter-clockwise from 3 o'clock (0 degrees).
        // For linear gradient, Android snaps to 45 degree intervals if specified in XML.
        float angleRad = (float)(fmod(mGradientState.angle, 360.0f) * M_PI / 180.0f);
        
        // Simple bounding box intersection for gradient line
        // Android's actual logic is a bit more complex (involving projecting the corners)
        // A simplified version based on angle:
        float x0 = mRect.left;
        float y0 = mRect.bottom;
        float x1 = mRect.right;
        float y1 = mRect.top;

        // Based on AOSP GradientDrawable orientation mapping (multiples of 45)
        int angleInt = (int)fmod(mGradientState.angle, 360.0f);
        if (angleInt < 0) angleInt += 360;

        switch (angleInt) {
            case 0:   x0 = mRect.left;  y0 = mRect.top;    x1 = mRect.right; y1 = mRect.top; break; // LEFT_RIGHT
            case 45:  x0 = mRect.left;  y0 = mRect.bottom; x1 = mRect.right; y1 = mRect.top; break; // BL_TR
            case 90:  x0 = mRect.left;  y0 = mRect.bottom; x1 = mRect.left;  y1 = mRect.top; break; // BOTTOM_TOP
            case 135: x0 = mRect.right; y0 = mRect.bottom; x1 = mRect.left;  y1 = mRect.top; break; // BR_TL
            case 180: x0 = mRect.right; y0 = mRect.top;    x1 = mRect.left;  y1 = mRect.top; break; // RIGHT_LEFT
            case 225: x0 = mRect.right; y0 = mRect.top;    x1 = mRect.left;  y1 = mRect.bottom; break; // TR_BL
            case 270: x0 = mRect.left;  y0 = mRect.top;    x1 = mRect.left;  y1 = mRect.bottom; break; // TOP_BOTTOM
            case 315: x0 = mRect.left;  y0 = mRect.top;    x1 = mRect.right; y1 = mRect.bottom; break; // TL_BR
            default:
                // Fallback for non-45 degree multiples
                float r = sqrt(w*w + h*h) / 2.0f;
                x0 = mRect.centerX() - r * cos(angleRad);
                y0 = mRect.centerY() + r * sin(angleRad);
                x1 = mRect.centerX() + r * cos(angleRad);
                y1 = mRect.centerY() - r * sin(angleRad);
                break;
        }

        mShader = std::make_shared<LinearGradient>(x0, y0, x1, y1, colors, positions, mGradientState.tileMode);
    } else if (mGradientState.type == GradientType::RADIAL) {
        float cx = mRect.left + mRect.width() * mGradientState.centerX;
        float cy = mRect.top + mRect.height() * mGradientState.centerY;
        float r = mGradientState.gradientRadius;
        if (r <= 0.0f) {
            r = std::min(mRect.width(), mRect.height()) * 0.5f; // Fallback
        }
        mShader = std::make_shared<RadialGradient>(cx, cy, r, colors, positions, mGradientState.tileMode);
    } else if (mGradientState.type == GradientType::SWEEP) {
        float cx = mRect.left + mRect.width() * mGradientState.centerX;
        float cy = mRect.top + mRect.height() * mGradientState.centerY;
        mShader = std::make_shared<SweepGradient>(cx, cy, colors, positions);
    }
}

void GradientDrawable::onBoundsChange(const Rect& bounds) {
    ensureValidRect();
    updateShader();
    mPathDirty = true;
}

void GradientDrawable::ensureValidRect() {
    const Rect& b = getBounds();
    // AOSP insets the fill by half the stroke width so that the stroke straddles
    // the fill's edge. Without this the stroke paints outward and the shape ends
    // up strokeWidth/2 larger than the view on every side.
    const float inset = hasStroke() ? mStrokeWidth * 0.5f : 0.0f;
    mRect.set((float)b.left + inset, (float)b.top + inset,
              (float)b.right - inset, (float)b.bottom - inset);
}

void GradientDrawable::draw(Canvas& canvas) {
    if (mRect.isEmpty()) return;

    const bool stroking = hasStroke();
    const uint32_t fillColor = applyAlpha(mSolidColor);
    const bool filling = (mHasSolid && (fillColor >> 24) != 0) || mHasGradient;
    if (!filling && !stroking) return;

    Paint fillPaint;
    fillPaint.setStyle(Style::FILL);
    if (mHasGradient) {
        // When drawing a gradient, solid color acts as fallback/tint if we were doing tinting,
        // but generally gradient overrides. We need to pass alpha down.
        // Wait, applyAlpha on mShader's colors is hard since Shader holds colors, so we just
        // rely on Canvas/Paint applying alpha? Direct2D doesn't automatically alpha the brush unless set.
        // Since we don't have Paint alpha yet, we'll just ignore drawable alpha for gradients for now.
        fillPaint.setShader(mShader);
    } else {
        fillPaint.setColor(fillColor);
    }

    Paint strokePaint;
    strokePaint.setStyle(Style::STROKE);
    strokePaint.setColor(applyAlpha(mStrokeColor));
    strokePaint.setStrokeWidth(mStrokeWidth);

    switch (mShape) {
        case Shape::RECTANGLE: {
            if (mHasRadiusArray) {
                if (mPathDirty) {
                    mPath.reset();
                    mPath.addRoundRect(mRect, mRadiusArray);
                    mPathDirty = false;
                }
                if (filling) canvas.drawPath(mPath, fillPaint);
                if (stroking) canvas.drawPath(mPath, strokePaint);
            } else if (mRadius > 0.0f) {
                // Clamp against the shorter side: a 100dp radius on a 40dp-tall
                // view is a pill, not a lens.
                const float rad = (std::min)(
                    mRadius, (std::min)(mRect.width(), mRect.height()) * 0.5f);
                if (filling) {
                    canvas.drawRoundRect(mRect.left, mRect.top, mRect.right, mRect.bottom,
                                         rad, rad, fillPaint);
                }
                if (stroking) {
                    canvas.drawRoundRect(mRect.left, mRect.top, mRect.right, mRect.bottom,
                                         rad, rad, strokePaint);
                }
            } else {
                if (filling) {
                    canvas.drawRect(mRect.left, mRect.top, mRect.right, mRect.bottom, fillPaint);
                }
                if (stroking) {
                    canvas.drawRect(mRect.left, mRect.top, mRect.right, mRect.bottom, strokePaint);
                }
            }
            break;
        }
        case Shape::OVAL: {
            if (mPathDirty) {
                mPath.reset();
                mPath.addOval(mRect);
                mPathDirty = false;
            }
            if (filling) canvas.drawPath(mPath, fillPaint);
            if (stroking) canvas.drawPath(mPath, strokePaint);
            break;
        }
        case Shape::LINE: {
            // Stroke only, across the vertical centre. AOSP draws no fill for a
            // line, so a <shape android:shape="line"> carrying only a <solid> is
            // invisible on a real device too - not a gap here.
            //
            // Note the empty-rect bail at the top of this function: a line in a
            // view exactly as tall as its own stroke insets to zero height and
            // draws nothing. AOSP's ensureValidRect does the same.
            if (stroking) {
                const float y = mRect.centerY();
                canvas.drawLine(mRect.left, y, mRect.right, y, strokePaint);
            }
            break;
        }
        case Shape::RING: {
            if (mPathDirty) {
                buildRingPath();
                mPathDirty = false;
            }
            if (filling) canvas.drawPath(mPath, fillPaint);
            if (stroking) canvas.drawPath(mPath, strokePaint);
            break;
        }
    }
}

} // namespace graphics
} // namespace setu
