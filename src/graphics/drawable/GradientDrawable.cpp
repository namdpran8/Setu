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

void GradientDrawable::onBoundsChange(const Rect& bounds) {
    ensureValidRect();
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
    const bool filling = mHasSolid && (fillColor >> 24) != 0;
    if (!filling && !stroking) return;

    Paint fillPaint;
    fillPaint.setStyle(Style::FILL);
    fillPaint.setColor(fillColor);

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
        default:
            // OVAL, LINE and RING arrive with the rest of the <shape> surface
            // area. Falling back to the rectangle keeps them visible in the
            // meantime rather than dropping the background entirely.
            if (filling) {
                canvas.drawRect(mRect.left, mRect.top, mRect.right, mRect.bottom, fillPaint);
            }
            if (stroking) {
                canvas.drawRect(mRect.left, mRect.top, mRect.right, mRect.bottom, strokePaint);
            }
            break;
    }
}

} // namespace graphics
} // namespace setu
