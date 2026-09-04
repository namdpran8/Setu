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

#include "BitmapDrawable.h"

#include <cmath>
#include <string>
#include <vector>

#include "../Canvas.h"
#include "../../utils/Logger.h"

namespace setu {
namespace graphics {

namespace {

constexpr const char* kTag = "BitmapDrawable";

// The ceiling on emulated tiling, in tiles. Without a shader every tile is its own
// drawBitmap, so a 1x1 pattern stretched over a full-screen view would be a million
// draw calls per frame; a hostile or merely careless APK should not be able to spend
// the frame budget that way. Past this, drawTiled stretches one copy over the bounds
// and logs - which for the tiny tiles that get anywhere near the ceiling is the
// average colour of the pattern, and therefore close to what the pattern looks like
// at that scale anyway.
constexpr size_t kMaxTiles = 4096;

// One tile along one axis. Source coordinates are bitmap pixels, destination
// coordinates are the canvas's own - the same space getBounds() is in.
struct TileSpan {
    float dstLo;
    float dstHi;
    float srcLo;
    float srcHi;
    bool flip;  // draw reversed about this span's own centre
};

// One axis of android.view.Gravity.apply. Taking the shift rather than pre-masked
// bits is what lets the same arithmetic serve x and y; an if-chain rather than a
// switch because the case values depend on that runtime shift.
//
// view::Gravity has applyHorizontal/applyVertical already, but neither can be used
// here: they centre FILL instead of stretching to the container, and FILL is this
// drawable's default gravity - so a background would centre at its intrinsic size
// instead of filling its view.
void applyGravityAxis(int gravity, int shift, int size, int containerLo, int containerHi,
                      int& outLo, int& outHi) {
    const int before = view::Gravity::AXIS_PULL_BEFORE << shift;
    const int after = view::Gravity::AXIS_PULL_AFTER << shift;
    const int clip = view::Gravity::AXIS_CLIP << shift;
    const bool clipping = (gravity & clip) == clip;

    const int pull = gravity & (before | after);
    if (pull == (before | after)) {
        // Both edges pull: FILL. Nothing to place, the content takes the container.
        outLo = containerLo;
        outHi = containerHi;
    } else if (pull == before) {
        outLo = containerLo;
        outHi = outLo + size;
        if (clipping && outHi > containerHi) outHi = containerHi;
    } else if (pull == after) {
        outHi = containerHi;
        outLo = outHi - size;
        if (clipping && outLo < containerLo) outLo = containerLo;
    } else {
        // Neither edge pulls: centred. Note this is also where a gravity of 0
        // lands, matching Gravity.apply - NO_GRAVITY centres, it does not go to the
        // top-left.
        outLo = containerLo + (containerHi - containerLo - size) / 2;
        outHi = outLo + size;
        if (clipping) {
            if (outLo < containerLo) outLo = containerLo;
            if (outHi > containerHi) outHi = containerHi;
        }
    }
}

// Fills `out` with the spans covering [lo, hi] on one axis. `tile` is the
// density-scaled on-screen size of one tile, `source` the same edge in bitmap
// pixels. Returns false, having produced nothing, when the range would need more
// than maxSpans tiles - so the caller can fall back without this having allocated
// them first.
//
// The tile grid is anchored at canvas coordinate 0, not at `lo`. That looks like a
// bug and is not: AOSP tiles through a BitmapShader whose local matrix is identity
// apart from density scale, and a shader's texture origin sits at the origin of the
// canvas's current coordinate space, not at the left edge of the rect being filled.
// Bounds that do not start at 0 therefore show a phase-shifted pattern in AOSP too.
bool buildSpans(BitmapDrawable::TileMode mode, float lo, float hi, float tile, float source,
                size_t maxSpans, std::vector<TileSpan>& out) {
    using TileMode = BitmapDrawable::TileMode;

    out.clear();
    if (tile <= 0.0f || source <= 0.0f || hi <= lo) return false;

    if (mode == TileMode::REPEAT || mode == TileMode::MIRROR) {
        if ((hi - lo) / tile + 2.0f > (float)maxSpans) return false;

        const int first = (int)std::floor(lo / tile);
        const int last = (int)std::ceil(hi / tile);  // exclusive
        for (int k = first; k < last; ++k) {
            // Mirror has period 2*tile: even tiles run forwards, odd ones backwards.
            // The double modulo is for negative k, which happens whenever the bounds
            // start left of the canvas origin.
            const bool flip = mode == TileMode::MIRROR && (((k % 2) + 2) % 2) == 1;
            out.push_back({(float)k * tile, (float)(k + 1) * tile, 0.0f, source, flip});
        }
        return true;
    }

    // CLAMP (and a DISABLED axis, which effectiveTileMode* has already turned into
    // CLAMP): the image once, with its edge pixel extended outwards. A shader does
    // that by clamping the texture coordinate; here it is a one-pixel-wide slice of
    // source stretched over the remainder, which samples to the same colours.
    if (lo < 0.0f) out.push_back({lo, 0.0f, 0.0f, 1.0f, false});
    out.push_back({0.0f, tile, 0.0f, source, false});
    if (hi > tile) out.push_back({tile, hi, source - 1.0f, source, false});
    return true;
}

} // namespace

BitmapDrawable::BitmapDrawable(std::shared_ptr<const Bitmap> bitmap)
    : mBitmap(std::move(bitmap)) {
    computeBitmapSize();
}

void BitmapDrawable::setBitmap(std::shared_ptr<const Bitmap> bitmap) {
    if (mBitmap == bitmap) return;
    mBitmap = std::move(bitmap);
    computeBitmapSize();
    invalidateSelf();
}

void BitmapDrawable::setTargetDensity(int densityDpi) {
    // AOSP reads 0 as "unspecified" here and substitutes the baseline, rather than
    // letting it reach scaleFromDensity - where DENSITY_NONE would instead mean
    // "do not scale at all".
    const int resolved = densityDpi == 0 ? Bitmap::DENSITY_DEFAULT : densityDpi;
    if (mTargetDensity == resolved) return;
    mTargetDensity = resolved;
    computeBitmapSize();
    invalidateSelf();
}

void BitmapDrawable::computeBitmapSize() {
    if (mBitmap) {
        mBitmapWidth = mBitmap->getScaledWidth(mTargetDensity);
        mBitmapHeight = mBitmap->getScaledHeight(mTargetDensity);
    } else {
        mBitmapWidth = -1;
        mBitmapHeight = -1;
    }
    // The size is half of where gravity puts the image, so this invalidates the
    // placement as surely as a bounds change does.
    mDstRectDirty = true;
}

void BitmapDrawable::setGravity(int gravity) {
    if (mGravity == gravity) return;
    mGravity = gravity;
    mDstRectDirty = true;
    invalidateSelf();
}

void BitmapDrawable::setTileModeX(TileMode mode) {
    setTileModeXY(mode, mTileModeY);
}

void BitmapDrawable::setTileModeY(TileMode mode) {
    setTileModeXY(mTileModeX, mode);
}

void BitmapDrawable::setTileModeXY(TileMode xMode, TileMode yMode) {
    if (mTileModeX == xMode && mTileModeY == yMode) return;
    mTileModeX = xMode;
    mTileModeY = yMode;
    // Turning tiling on or off switches between a gravity-placed rect and the whole
    // bounds, so the destination has to be recomputed either way.
    mDstRectDirty = true;
    invalidateSelf();
}

BitmapDrawable::TileMode BitmapDrawable::parseTileMode(int attrValue) {
    switch (attrValue) {
        case (int)TileMode::CLAMP:
            return TileMode::CLAMP;
        case (int)TileMode::REPEAT:
            return TileMode::REPEAT;
        case (int)TileMode::MIRROR:
            return TileMode::MIRROR;
        default:
            // TILE_MODE_UNDEFINED, TILE_MODE_DISABLED, and anything AAPT should
            // never have produced. AOSP returns null for all three.
            return TileMode::DISABLED;
    }
}

void BitmapDrawable::setAntiAlias(bool antiAlias) {
    if (mAntiAlias == antiAlias) return;
    mAntiAlias = antiAlias;
    invalidateSelf();
}

void BitmapDrawable::setFilterBitmap(bool filter) {
    if (mFilter == filter) return;
    mFilter = filter;
    invalidateSelf();
}

void BitmapDrawable::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mAlpha == alpha) return;
    mAlpha = alpha;
    invalidateSelf();
}

void BitmapDrawable::onBoundsChange(const Rect& bounds) {
    mDstRectDirty = true;
}

void BitmapDrawable::updateDstRectIfDirty() {
    if (!mDstRectDirty) return;

    const Rect& bounds = getBounds();
    if (isTiled()) {
        // AOSP's copyBounds(mDstRect): tiles cover the bounds, gravity does not
        // apply.
        mDstRect = bounds;
    } else {
        // START/END resolved LTR - see the note on view::Gravity::getAbsoluteGravity
        // about what threading a real layout direction through here would take.
        const int gravity = view::Gravity::getAbsoluteGravity(mGravity);
        applyGravityAxis(gravity, view::Gravity::AXIS_X_SHIFT, mBitmapWidth, bounds.left,
                         bounds.right, mDstRect.left, mDstRect.right);
        applyGravityAxis(gravity, view::Gravity::AXIS_Y_SHIFT, mBitmapHeight, bounds.top,
                         bounds.bottom, mDstRect.top, mDstRect.bottom);
    }

    mDstRectDirty = false;
}

void BitmapDrawable::draw(Canvas& canvas) {
    if (!mBitmap) return;

    if (getBounds().isEmpty()) return;

    // A fully transparent image still costs a display-list entry and a sampler pass.
    // Skipped here the way ColorDrawable skips a transparent fill.
    if (mAlpha == 0) return;

    updateDstRectIfDirty();
    if (mDstRect.isEmpty()) return;

    // Only the alpha of this reaches the backend, per the contract on
    // Canvas::drawBitmap - tinting an image needs a ColorFilter, which does not
    // exist here yet. The RGB is set to white rather than left at Paint's default
    // black so that a backend which ever does start modulating by the colour
    // multiplies by 1 instead of 0.
    Paint paint;
    paint.setColor(((uint32_t)mAlpha << 24) | 0x00FFFFFFu);
    paint.setAntiAlias(mAntiAlias);
    paint.setColorFilter(getActiveColorFilter());

    if (isTiled()) {
        drawTiled(canvas, paint);
        return;
    }

    // An empty src is Canvas::drawBitmap's spelling of Android's null Rect: the
    // whole bitmap. The dst/src mismatch is what applies the density scale, since
    // mDstRect was sized from the scaled intrinsic size.
    canvas.drawBitmap(*mBitmap, RectF(), RectF(mDstRect), paint);
}

void BitmapDrawable::drawTiled(Canvas& canvas, const Paint& paint) {
    // A tile is the density-scaled size, which is what AOSP's
    // postScale(targetDensity / sourceDensity) on the shader matrix produces: one
    // tile is exactly as big on screen as the intrinsic size this drawable reports.
    const float tileW = (float)mBitmapWidth;
    const float tileH = (float)mBitmapHeight;
    const float srcW = (float)mBitmap->getWidth();
    const float srcH = (float)mBitmap->getHeight();

    const RectF dst(mDstRect);

    std::vector<TileSpan> spansX;
    std::vector<TileSpan> spansY;
    const bool built = buildSpans(effectiveTileModeX(), dst.left, dst.right, tileW, srcW,
                                 kMaxTiles, spansX) &&
                       buildSpans(effectiveTileModeY(), dst.top, dst.bottom, tileH, srcH,
                                  kMaxTiles, spansY);

    if (!built || spansX.size() * spansY.size() > kMaxTiles) {
        // Degenerate geometry, or more tiles than the ceiling allows. Either way one
        // stretched copy is the graceful answer - and for a pattern this small it is
        // very nearly the right one.
        Logger::w(kTag, "tileMode needs more than " + std::to_string(kMaxTiles) +
                            " tiles for a " + std::to_string(mBitmapWidth) + "x" +
                            std::to_string(mBitmapHeight) + " image in " +
                            std::to_string(mDstRect.width()) + "x" +
                            std::to_string(mDstRect.height()) + "; stretching one copy");
        canvas.drawBitmap(*mBitmap, RectF(), dst, paint);
        return;
    }

    // REPEAT and MIRROR spans overhang the bounds by up to one tile on each side.
    // Clipping once and drawing whole tiles is both cheaper and much harder to get
    // wrong than narrowing every edge tile in source space - and a partial tile has
    // to be a *slice* of the image, which is exactly what the clip leaves.
    canvas.save();
    canvas.clipRect(dst.left, dst.top, dst.right, dst.bottom);

    for (const TileSpan& sy : spansY) {
        for (const TileSpan& sx : spansX) {
            const RectF src(sx.srcLo, sy.srcLo, sx.srcHi, sy.srcHi);
            const RectF tile(sx.dstLo, sy.dstLo, sx.dstHi, sy.dstHi);

            if (!sx.flip && !sy.flip) {
                canvas.drawBitmap(*mBitmap, src, tile, paint);
                continue;
            }

            // Mirror in place. Canvas composes these so that the point is scaled
            // first and translated second, so translating by (lo + hi) and scaling
            // by -1 maps lo to hi and hi to lo: the tile stays where it is and comes
            // out reversed. Same two calls AOSP uses for its RTL mirroring.
            canvas.save();
            canvas.translate(sx.flip ? tile.left + tile.right : 0.0f,
                             sy.flip ? tile.top + tile.bottom : 0.0f);
            canvas.scale(sx.flip ? -1.0f : 1.0f, sy.flip ? -1.0f : 1.0f);
            canvas.drawBitmap(*mBitmap, src, tile, paint);
            canvas.restore();
        }
    }

    canvas.restore();
}

} // namespace graphics
} // namespace setu
