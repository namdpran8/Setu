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

#include <cstdint>

#include "Drawable.h"
#include "../Paint.h"
#include "../Path.h"
#include "../ColorStateList.h"

namespace setu {
namespace graphics {

// android.graphics.drawable.GradientDrawable: everything a <shape> element can
// describe.
//
// This is the single most common drawable in real layouts - rounded buttons,
// card outlines, dividers, chip backgrounds - so getting its geometry right is
// most of what makes an app look like itself. Two AOSP details are load-bearing
// and both are easy to miss:
//
//   * the fill is inset by half the stroke width, so a stroke straddles the edge
//     of the fill rather than sitting outside it (ensureValidRect);
//   * the corner radius is clamped to half the shorter side, otherwise a short
//     wide view with a large radius renders as a thin lens instead of a pill.
//
// Resource parsing lives in ui/DrawableInflater; this class only knows geometry
// and colour, and is driven entirely through setters.
class GradientDrawable : public Drawable {
public:
    // Values match android.graphics.drawable.GradientDrawable's constants, which
    // are also the enum ordinals AAPT compiles android:shape down to.
    enum class Shape : int {
        RECTANGLE = 0,
        OVAL = 1,
        LINE = 2,
        RING = 3
    };

    GradientDrawable() = default;

    // AOSP's DEFAULT_INNER_RADIUS_RATIO / DEFAULT_THICKNESS_RATIO, used when a
    // ring gives no absolute innerRadius / thickness.
    static constexpr float DEFAULT_INNER_RADIUS_RATIO = 3.0f;
    static constexpr float DEFAULT_THICKNESS_RATIO = 9.0f;

    void draw(Canvas& canvas) override;

    void setShape(Shape shape);
    Shape getShape() const { return mShape; }

    // <solid android:color>. Clearing it leaves an outline-only shape.
    void setColor(uint32_t argb);
    void clearColor();
    bool hasColor() const { return mHasSolid; }
    uint32_t getColor() const { return mSolidColor; }

    // <solid android:color="@color/some_selector">. The fill follows the owner's
    // state, which is what a stock Material button background does: one <shape>
    // whose solid is a colour selector, rather than a <selector> of two <shape>s.
    // Passing null goes back to a flat colour.
    //
    // Sets the current fill immediately from the state already pushed in, so a
    // drawable that is given its list after its state still paints correctly.
    void setColor(const ColorStateListPtr& csl);
    const ColorStateListPtr& getColorStateList() const { return mSolidCsl; }

    // <stroke android:color="@color/some_selector">. Same rules as the fill.
    void setStrokeColorStateList(const ColorStateListPtr& csl);

    // <corners android:radius>
    void setCornerRadius(float radius);
    float getCornerRadius() const { return mRadius; }

    // <corners> with per-corner values, in Android's Path order:
    // topLeftX/Y, topRightX/Y, bottomRightX/Y, bottomLeftX/Y.
    // Passing nullptr goes back to the uniform radius.
    void setCornerRadii(const float radii[8]);
    bool hasCornerRadii() const { return mHasRadiusArray; }

    // <stroke>. A width of 0 or less removes the stroke.
    void setStroke(float width, uint32_t color);
    void setStroke(float width, uint32_t color, float dashWidth, float dashGap);
    float getStrokeWidth() const { return mStrokeWidth; }

    // <shape android:innerRadius|innerRadiusRatio|thickness|thicknessRatio>, for
    // Shape::RING. -1 on either absolute dimension is AOSP's sentinel for "derive
    // this from the matching ratio instead", and is the default for both.
    void setRingGeometry(float innerRadius, float innerRadiusRatio,
                         float thickness, float thicknessRatio);

    // <padding>. Reported back to the owning View through getPadding().
    void setPaddingInsets(int left, int top, int right, int bottom);
    bool getPadding(Rect& padding) const override;

    // <size>. -1 on either axis means "no intrinsic size on that axis".
    void setSize(int width, int height);
    int getIntrinsicWidth() const override { return mWidth; }
    int getIntrinsicHeight() const override { return mHeight; }

    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

    // True when either the fill or the stroke is driven by a colour selector.
    // Only then does the owner push its drawable state in here.
    bool isStateful() const override;

protected:
    void onBoundsChange(const Rect& bounds) override;

    // Re-resolves the fill and stroke colours for the new state. Returns true
    // when either changed, so the owner knows to repaint.
    bool onStateChange(const std::vector<int>& stateSet) override;

private:
    // AOSP's ensureValidRect: the drawing rect, inset by half the stroke.
    void ensureValidRect();

    // Builds the annulus for Shape::RING into mPath, following AOSP's buildRing.
    void buildRingPath();

    // Applies mAlpha on top of an authored colour.
    uint32_t applyAlpha(uint32_t argb) const;

    bool hasStroke() const { return mStrokeWidth > 0.0f && (mStrokeColor >> 24) != 0; }

    Shape mShape = Shape::RECTANGLE;

    bool mHasSolid = false;
    uint32_t mSolidColor = 0x00000000;

    // The authored colour selectors, when the fill or stroke was given one.
    // mSolidColor and mStrokeColor above stay the single source of truth for
    // what actually paints - these only re-derive them on a state change, so
    // draw() never has to know whether a selector was involved.
    ColorStateListPtr mSolidCsl;
    ColorStateListPtr mStrokeCsl;

    float mRadius = 0.0f;
    bool mHasRadiusArray = false;
    float mRadiusArray[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    float mStrokeWidth = -1.0f;
    uint32_t mStrokeColor = 0x00000000;
    // Parsed and kept so the information is not lost, but not yet honoured:
    // Paint has no dash support, so a dashed stroke currently draws solid.
    float mStrokeDashWidth = 0.0f;
    float mStrokeDashGap = 0.0f;

    // Ring geometry, used only by Shape::RING. The ratio defaults are AOSP's
    // DEFAULT_INNER_RADIUS_RATIO and DEFAULT_THICKNESS_RATIO; an absolute value of
    // -1 means the corresponding ratio is what applies.
    float mInnerRadius = -1.0f;
    float mInnerRadiusRatio = DEFAULT_INNER_RADIUS_RATIO;
    float mThickness = -1.0f;
    float mThicknessRatio = DEFAULT_THICKNESS_RATIO;

    Rect mPaddingInsets;
    bool mHasPadding = false;

    int mWidth = -1;
    int mHeight = -1;

    int mAlpha = 255;

    // Recomputed on bounds/stroke change rather than per draw call.
    RectF mRect;
    Path mPath;
    bool mPathDirty = true;
};

} // namespace graphics
} // namespace setu
