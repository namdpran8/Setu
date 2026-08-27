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

#include "NinePatchDrawable.h"

#include "androidfw/ResourceTypes.h"

#include <cmath>
#include <string>

#include "../Canvas.h"
#include "../Paint.h"
#include "../../utils/Logger.h"

namespace setu {
namespace graphics {

namespace {

constexpr const char* kTag = "NinePatchDrawable";

// AOSP's Drawable.scaleFromDensity with isSize=true, which differs from Bitmap's own
// in the one way that matters here: both round to nearest, but this one refuses to
// round a non-zero input down to zero.
//
// That clamp is the whole reason for not reusing Bitmap::getScaledWidth for padding:
// a 1px inset on an xxhdpi asset drawn on an mdpi screen is 1 * 160/480 = 0.33, and
// rounding that to 0 would let a button's label sit flush against its artwork.
int scaleSizeFromDensity(int size, int sourceDensity, int targetDensity) {
    if (size == 0 || sourceDensity == Bitmap::DENSITY_NONE ||
        targetDensity == Bitmap::DENSITY_NONE || sourceDensity == targetDensity) {
        return size;
    }

    const float scaled = (float)size * (float)targetDensity / (float)sourceDensity;
    const int rounded = (int)std::lround(scaled);
    if (rounded != 0) return rounded;
    return size > 0 ? 1 : -1;
}

} // namespace

NinePatchDrawable::NinePatchDrawable(std::shared_ptr<const Bitmap> bitmap,
                                     const std::vector<uint8_t>& ninePatchChunk) {
    setNinePatch(std::move(bitmap), ninePatchChunk);
}

bool NinePatchDrawable::setNinePatch(std::shared_ptr<const Bitmap> bitmap,
                                     const std::vector<uint8_t>& ninePatchChunk) {
    mBitmap = std::move(bitmap);
    mChunkValid = parseChunk(ninePatchChunk);
    computeBitmapSize();
    mLatticeDirty = true;
    invalidateSelf();
    return mChunkValid;
}

bool NinePatchDrawable::parseChunk(const std::vector<uint8_t>& chunk) {
    mXDivs.clear();
    mYDivs.clear();
    mCellColors.clear();
    mSourcePadding.setEmpty();

    if (!mBitmap) return false;

    if (chunk.size() < sizeof(android::Res_png_9patch)) {
        Logger::w(kTag, "npTc chunk is " + std::to_string(chunk.size()) + " bytes, need at least " +
                            std::to_string(sizeof(android::Res_png_9patch)));
        return false;
    }

    // The same cast BitmapFactory does on its way out. Reading through a const
    // pointer, because deserialize() has already run and there is nothing left to
    // fill in - see the note in the header about why re-running it is not an option.
    const auto* patch = reinterpret_cast<const android::Res_png_9patch*>(chunk.data());

    if (!patch->wasDeserialized) {
        Logger::w(kTag, "npTc chunk has not been deserialized; expected a chunk from "
                        "BitmapFactory::Options::outNinePatchChunk");
        return false;
    }

    const size_t numX = patch->numXDivs;
    const size_t numY = patch->numYDivs;
    const size_t numC = patch->numColors;

    const size_t header = sizeof(android::Res_png_9patch);
    const size_t need = header + (numX + numY) * sizeof(int32_t) + numC * sizeof(uint32_t);
    if (chunk.size() < need) {
        Logger::w(kTag, "npTc chunk is " + std::to_string(chunk.size()) + " bytes, need " +
                            std::to_string(need) + " for " + std::to_string(numX) + "x" +
                            std::to_string(numY) + " divs and " + std::to_string(numC) +
                            " colours");
        return false;
    }

    // The offsets have to be the ones fill9patchOffsets computes. Checking them
    // rather than trusting them is what makes getXDivs()'s arithmetic - which indexes
    // off the struct's own address - safe to run against a buffer this class did not
    // produce.
    if (patch->xDivsOffset != header ||
        patch->yDivsOffset != patch->xDivsOffset + numX * sizeof(int32_t) ||
        patch->colorsOffset != patch->yDivsOffset + numY * sizeof(int32_t)) {
        Logger::w(kTag, "npTc chunk offsets are not the deserialized layout");
        return false;
    }

    const int srcWidth = mBitmap->getWidth();
    const int srcHeight = mBitmap->getHeight();

    // Divs are in the coordinates of the *trimmed* image: aapt strips the one-pixel
    // marker border when it compiles a .9.png, so these index straight into the
    // bitmap with no offset to undo.
    const int32_t* xDivs = patch->getXDivs();
    const int32_t* yDivs = patch->getYDivs();

    int32_t previous = 0;
    for (size_t i = 0; i < numX; ++i) {
        if (xDivs[i] < previous || xDivs[i] > srcWidth) {
            Logger::w(kTag, "npTc xDiv " + std::to_string(i) + " is " +
                                std::to_string(xDivs[i]) + ", outside [" +
                                std::to_string(previous) + ", " + std::to_string(srcWidth) + "]");
            mXDivs.clear();
            return false;
        }
        previous = xDivs[i];
        mXDivs.push_back(xDivs[i]);
    }

    previous = 0;
    for (size_t i = 0; i < numY; ++i) {
        if (yDivs[i] < previous || yDivs[i] > srcHeight) {
            Logger::w(kTag, "npTc yDiv " + std::to_string(i) + " is " +
                                std::to_string(yDivs[i]) + ", outside [" +
                                std::to_string(previous) + ", " + std::to_string(srcHeight) + "]");
            mXDivs.clear();
            mYDivs.clear();
            return false;
        }
        previous = yDivs[i];
        mYDivs.push_back(yDivs[i]);
    }

    // One colour per cell, row-major. A mismatch is not worth rejecting the whole
    // chunk over - the colours are only an optimisation - so the array is dropped and
    // every cell gets drawn.
    const size_t cells = (numX + 1) * (numY + 1);
    if (numC == cells) {
        const uint32_t* colors = patch->getColors();
        mCellColors.assign(colors, colors + numC);
    } else if (numC != 0) {
        Logger::d(kTag, "npTc has " + std::to_string(numC) + " colours for " +
                            std::to_string(cells) + " cells; ignoring them");
    }

    // Named fields, not positional: the struct declares these as
    // (paddingLeft, paddingRight, paddingTop, paddingBottom), which is *not* Rect's
    // l/t/r/b order. Reading them in struct order would swap top with right.
    mSourcePadding.set(patch->paddingLeft < 0 ? 0 : patch->paddingLeft,
                       patch->paddingTop < 0 ? 0 : patch->paddingTop,
                       patch->paddingRight < 0 ? 0 : patch->paddingRight,
                       patch->paddingBottom < 0 ? 0 : patch->paddingBottom);

    return true;
}

void NinePatchDrawable::setTargetDensity(int densityDpi) {
    const int resolved = densityDpi == 0 ? Bitmap::DENSITY_DEFAULT : densityDpi;
    if (mTargetDensity == resolved) return;
    mTargetDensity = resolved;
    computeBitmapSize();
    mLatticeDirty = true;
    invalidateSelf();
}

void NinePatchDrawable::computeBitmapSize() {
    if (!mBitmap) {
        mBitmapWidth = -1;
        mBitmapHeight = -1;
        mPadding.setEmpty();
        mDensityScale = 1.0f;
        return;
    }

    mBitmapWidth = mBitmap->getScaledWidth(mTargetDensity);
    mBitmapHeight = mBitmap->getScaledHeight(mTargetDensity);

    // AOSP substitutes the target density for DENSITY_NONE here, which comes to the
    // same thing as not scaling at all.
    const int sourceDensity = mBitmap->getDensity();

    mPadding.set(scaleSizeFromDensity(mSourcePadding.left, sourceDensity, mTargetDensity),
                 scaleSizeFromDensity(mSourcePadding.top, sourceDensity, mTargetDensity),
                 scaleSizeFromDensity(mSourcePadding.right, sourceDensity, mTargetDensity),
                 scaleSizeFromDensity(mSourcePadding.bottom, sourceDensity, mTargetDensity));

    if (sourceDensity == Bitmap::DENSITY_NONE || mTargetDensity == Bitmap::DENSITY_NONE ||
        sourceDensity == mTargetDensity) {
        mDensityScale = 1.0f;
    } else {
        mDensityScale = (float)mTargetDensity / (float)sourceDensity;
    }
}

bool NinePatchDrawable::getPadding(Rect& padding) const {
    padding = mPadding;
    // AOSP's exact test. All-zero padding reads as "this drawable has none", which
    // leaves whatever the view already had in place rather than zeroing it.
    return (mPadding.left | mPadding.top | mPadding.right | mPadding.bottom) != 0;
}

void NinePatchDrawable::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mAlpha == alpha) return;
    mAlpha = alpha;
    invalidateSelf();
}

void NinePatchDrawable::onBoundsChange(const Rect& bounds) {
    mLatticeDirty = true;
}

// Skia's SkLatticeIter::set_points, which is what AOSP's NinePatch ends up running.
//
// `divs` are the division boundaries in source pixels; column i spans
// [div[i-1], div[i]) with 0 and `sourceSize` standing in at the ends, so n divs make
// n+1 columns. Odd-numbered columns are the stretchable ones: the run of marker
// pixels that produced div[0] starts the first stretchable region, so the column
// before it - column 0 - is fixed, and they alternate from there.
void NinePatchDrawable::buildSpans(const std::vector<int32_t>& divs, int sourceSize,
                                   float densityScale, float dstLo, float dstHi,
                                   std::vector<Span>& out) {
    out.clear();

    const size_t columns = divs.size() + 1;
    const auto sourceLo = [&](size_t i) { return i == 0 ? 0 : divs[i - 1]; };
    const auto sourceHi = [&](size_t i) {
        return i == divs.size() ? sourceSize : divs[i];
    };

    // Pass one: how much of the destination is spoken for by cells that must keep
    // their size, and how much is available to share out among the ones that stretch.
    float fixed = 0.0f;
    float scalable = 0.0f;
    for (size_t i = 0; i < columns; ++i) {
        const float extent = (float)(sourceHi(i) - sourceLo(i)) * densityScale;
        if (i % 2 == 1) {
            scalable += extent;
        } else {
            fixed += extent;
        }
    }

    const float dstLen = dstHi - dstLo;
    float fixedScale = 1.0f;
    float stretchScale = 0.0f;

    if (scalable <= 0.0f) {
        // Nothing on this axis is stretchable - a malformed patch, or a deliberately
        // rigid one. SkLatticeIter would divide by zero here; scaling the fixed cells
        // to fill is the sane reading, and degrades to the plain stretched bitmap
        // that the no-divs fallback wants anyway.
        fixedScale = fixed > 0.0f ? dstLen / fixed : 0.0f;
    } else if (fixed <= dstLen) {
        // The normal case: corners and borders at their authored size, the slack
        // divided among the stretchable cells in proportion to their source extents.
        stretchScale = (dstLen - fixed) / scalable;
    } else {
        // Too small even for the un-stretchable parts. Skia collapses the stretchable
        // cells to nothing and shrinks the fixed ones, which keeps the corner artwork
        // recognisable instead of clipping half of it away.
        fixedScale = dstLen / fixed;
    }

    // Pass two: lay them out, carrying the fractional cursor forward so that each
    // cell starts exactly where the previous one ended. That shared edge is what
    // keeps a stretched nine-patch from showing hairline seams; rounding each cell
    // independently is the classic way to produce them.
    float cursor = dstLo;
    for (size_t i = 0; i < columns; ++i) {
        const float lo = (float)sourceLo(i);
        const float hi = (float)sourceHi(i);
        const float extent = (hi - lo) * densityScale * (i % 2 == 1 ? stretchScale : fixedScale);

        Span span;
        span.srcLo = lo;
        span.srcHi = hi;
        span.dstLo = cursor;
        span.dstHi = cursor + extent;
        out.push_back(span);

        cursor += extent;
    }
}

void NinePatchDrawable::updateLatticeIfDirty() {
    if (!mLatticeDirty) return;
    mLatticeDirty = false;

    mCols.clear();
    mRows.clear();
    if (!mBitmap) return;

    const Rect& bounds = getBounds();

    // A rejected chunk leaves no divs, which buildSpans turns into a single fixed
    // column scaled to fill - i.e. the whole image stretched over the whole bounds,
    // which is the documented fallback. No special case needed for it.
    buildSpans(mXDivs, mBitmap->getWidth(), mDensityScale, (float)bounds.left,
               (float)bounds.right, mCols);
    buildSpans(mYDivs, mBitmap->getHeight(), mDensityScale, (float)bounds.top,
               (float)bounds.bottom, mRows);
}

void NinePatchDrawable::draw(Canvas& canvas) {
    if (!mBitmap) return;
    if (getBounds().isEmpty()) return;
    if (mAlpha == 0) return;

    updateLatticeIfDirty();
    if (mCols.empty() || mRows.empty()) return;

    // Only the alpha reaches the backend, per the contract on Canvas::drawBitmap.
    Paint paint;
    paint.setColor(((uint32_t)mAlpha << 24) | 0x00FFFFFFu);

    const size_t numCols = mCols.size();
    const bool useColors = !mCellColors.empty() && mCellColors.size() == numCols * mRows.size();

    for (size_t row = 0; row < mRows.size(); ++row) {
        const Span& r = mRows[row];
        // An empty source row is normal, not a bug: a stretch region that starts at
        // pixel 0 leaves a zero-width fixed cell before it. An empty destination row
        // is the collapsed-stretch case at minimum size.
        if (r.srcHi <= r.srcLo || r.dstHi <= r.dstLo) continue;

        for (size_t col = 0; col < numCols; ++col) {
            const Span& c = mCols[col];
            if (c.srcHi <= c.srcLo || c.dstHi <= c.dstLo) continue;

            // TRANSPARENT_COLOR means aapt found the cell fully transparent, so the
            // blit would contribute nothing. Skipping it is worth the lookup: the
            // rounded corners of a stock button background are exactly this.
            if (useColors && mCellColors[row * numCols + col] ==
                                 (uint32_t)android::Res_png_9patch::TRANSPARENT_COLOR) {
                continue;
            }

            canvas.drawBitmap(*mBitmap, RectF(c.srcLo, r.srcLo, c.srcHi, r.srcHi),
                              RectF(c.dstLo, r.dstLo, c.dstHi, r.dstHi), paint);
        }
    }
}

} // namespace graphics
} // namespace setu
