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
#include <memory>

#include "Drawable.h"
#include "../Bitmap.h"
#include "../Paint.h"
#include "../../view/Gravity.h"

namespace setu {
namespace graphics {

class Canvas;

// android.graphics.drawable.BitmapDrawable: draws one decoded image inside its
// bounds.
//
// This is the other half of what BitmapFactory started. The factory deliberately
// never resamples - it decodes at native pixel size and records the density the
// pixels were authored at - which leaves exactly one job here: turn (source
// density, target density) into an on-screen size. That is what
// getIntrinsicWidth/Height report, and it is why a drawable-hdpi asset ends up the
// same physical size as its drawable-xxhdpi sibling.
//
// Three things about the geometry are worth stating up front, because each is a
// place a bitmap silently draws at the wrong size rather than failing:
//
//   * The intrinsic size is the *scaled* size, cached. AOSP keeps it in
//     mBitmapWidth/mBitmapHeight instead of scaling per call, and so does this: a
//     View asks for it several times per measure pass, and the answer only moves
//     when the bitmap or the target density does.
//
//   * mTargetDensity starts at Bitmap::DENSITY_DEFAULT (160), which is AOSP's
//     default and is the wrong answer on any real screen. Whoever inflates one of
//     these has to push the display density in through setTargetDensity().
//     Forgetting to does not crash: a drawable-xxhdpi asset just comes out at a
//     third of its right size, because 160/480 is exactly what it asked for.
//
//   * The default gravity is FILL, not CENTER - so a BitmapDrawable used as a
//     background stretches to its view, and android:gravity is what stops it.
//
// The one deliberate divergence is tiling. AOSP hands the Paint a BitmapShader and
// lets Skia repeat the texture inside a single drawRect. There is no Shader in this
// runtime and Canvas cannot express one, so android:tileMode is emulated by drawing
// the image once per tile. That is exact, but linear in tile count instead of
// constant, which is why drawTiled has a ceiling (kMaxTiles, in the .cpp) and a
// documented fallback past it.
//
// Deliberately absent, so a later phase does not have to guess whether it was
// forgotten: android:mipMap and android:dither (neither has anywhere to land -
// Paint has no such flags, and D2D chooses its own sampling); android:autoMirrored
// and AOSP's needMirroring(), because this runtime is LTR-only (see view/Gravity);
// tinting, which needs the ColorFilter this runtime does not have yet; and
// nine-patch, which is a different class over the same decoded bytes.
class BitmapDrawable : public Drawable {
public:
    // The values are android.graphics.drawable.BitmapDrawable's own TILE_MODE_*
    // constants, which are also the enum ordinals AAPT compiles android:tileMode
    // down to - so an inflater can hand the raw attribute to parseTileMode.
    //
    // DISABLED stands in for AOSP's null TileMode. Null and CLAMP are genuinely
    // different things there: null means "do not tile on this axis at all", CLAMP
    // means "tile, extending the edge pixel outwards forever". They only coincide
    // when *both* axes are null, which is the case where AOSP clears the shader and
    // gravity takes over.
    enum class TileMode : int {
        DISABLED = -1,
        CLAMP = 0,
        REPEAT = 1,
        MIRROR = 2,
    };

    // What android:tileMode reads as when the attribute is absent. Callers must
    // test for it *before* calling a setter, the way AOSP's inflate() does: it is
    // what keeps an absent android:tileModeY from clearing the mode that an
    // android:tileMode on the same element just set.
    static constexpr int TILE_MODE_UNDEFINED = -2;

    BitmapDrawable() = default;
    explicit BitmapDrawable(std::shared_ptr<const Bitmap> bitmap);

    void draw(Canvas& canvas) override;

    // shared_ptr<const Bitmap> rather than a reference: a drawable outlives the
    // inflate that built it and must keep the pixels alive, and const because
    // drawing never writes them - Bitmap::getD2DBitmap is const for exactly this.
    // Passing nullptr is legal and draws nothing, which is the state an inflate
    // that could not decode leaves behind.
    void setBitmap(std::shared_ptr<const Bitmap> bitmap);
    const std::shared_ptr<const Bitmap>& getBitmap() const { return mBitmap; }

    // The density of the screen this will be drawn on, in DPI - 480 for an xxhdpi
    // display, not the 3.0 scale factor WindowManager::getDensity() returns. The
    // conversion at that boundary is dpi = round(scale * DENSITY_DEFAULT); mixing
    // the two scales everything by 160x. 0 is read as DENSITY_DEFAULT, matching
    // AOSP's setTargetDensity.
    void setTargetDensity(int densityDpi);
    int getTargetDensity() const { return mTargetDensity; }

    // The bitmap's size after density scaling, or -1 with no bitmap. Cached, per
    // the note above; recomputed by computeBitmapSize().
    int getIntrinsicWidth() const override { return mBitmapWidth; }
    int getIntrinsicHeight() const override { return mBitmapHeight; }

    // android:gravity. A view::Gravity bitfield, defaulting to FILL - which is why
    // it is a bitfield and not an enum: FILL is LEFT|RIGHT|TOP|BOTTOM, and
    // CENTER_HORIZONTAL|TOP is an ordinary value that no == comparison will match.
    //
    // Ignored while tiling, exactly as in AOSP: a tiled bitmap covers the bounds,
    // so there is nothing left for gravity to place.
    void setGravity(int gravity);
    int getGravity() const { return mGravity; }

    // android:tileMode / android:tileModeX / android:tileModeY.
    void setTileModeX(TileMode mode);
    void setTileModeY(TileMode mode);
    void setTileModeXY(TileMode xMode, TileMode yMode);
    TileMode getTileModeX() const { return mTileModeX; }
    TileMode getTileModeY() const { return mTileModeY; }

    // The attribute value as AAPT compiled it, turned into a TileMode. Anything
    // unrecognised - including TILE_MODE_UNDEFINED - becomes DISABLED, which is
    // AOSP's null return from the same switch.
    static TileMode parseTileMode(int attrValue);

    // android:antialias. Stored and forwarded to the Paint that draw() builds,
    // which is what AOSP does with it. Be aware that it currently changes nothing
    // on screen: Direct2DCanvas::drawBitmap samples LINEAR and D2D's antialias mode
    // is a device-level setting for geometry edges, not image edges.
    void setAntiAlias(bool antiAlias);
    bool hasAntiAlias() const { return mAntiAlias; }

    // android:filter. Kept so the information is not lost at inflate time, but not
    // yet honoured: Canvas::drawBitmap has no way to ask for a sampling mode, and
    // Direct2DCanvas always draws D2D1_INTERPOLATION_MODE_LINEAR. So a
    // deliberately-pixelated asset (android:filter="false") currently smooths.
    // Honouring it means an interpolation argument on Canvas::drawBitmap, which is
    // a change to every canvas and therefore not this class's to make.
    void setFilterBitmap(bool filter);
    bool isFilterBitmap() const { return mFilter; }

    // 0..255, multiplied into the image. This is the only channel of the Paint that
    // reaches the backend, per the contract on Canvas::drawBitmap.
    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

protected:
    void onBoundsChange(const Rect& bounds) override;

private:
    // AOSP's computeBitmapSize: the cached intrinsic size, or -1/-1 with no bitmap.
    void computeBitmapSize();

    // AOSP's updateDstRectAndInsetsIfDirty, minus the optical insets - Drawable has
    // no getOpticalInsets() hook to report them through. Recomputed on a bounds,
    // gravity, tile-mode or bitmap change rather than per draw call.
    void updateDstRectIfDirty();

    void drawTiled(Canvas& canvas, const Paint& paint);

    bool isTiled() const {
        return mTileModeX != TileMode::DISABLED || mTileModeY != TileMode::DISABLED;
    }

    // While tiling, a DISABLED axis becomes CLAMP - which is the substitution AOSP
    // makes when it builds the BitmapShader out of one null mode and one real one.
    TileMode effectiveTileModeX() const {
        return mTileModeX == TileMode::DISABLED ? TileMode::CLAMP : mTileModeX;
    }
    TileMode effectiveTileModeY() const {
        return mTileModeY == TileMode::DISABLED ? TileMode::CLAMP : mTileModeY;
    }

    std::shared_ptr<const Bitmap> mBitmap;

    int mTargetDensity = Bitmap::DENSITY_DEFAULT;

    // Density-scaled, cached. -1 means "no intrinsic size", the same sentinel
    // Drawable::getIntrinsicWidth returns by default.
    int mBitmapWidth = -1;
    int mBitmapHeight = -1;

    int mGravity = view::Gravity::FILL;

    TileMode mTileModeX = TileMode::DISABLED;
    TileMode mTileModeY = TileMode::DISABLED;

    bool mAntiAlias = false;  // AOSP's BitmapState paint starts with AA off
    bool mFilter = true;      // ... and DEFAULT_PAINT_FLAGS has FILTER_BITMAP_FLAG on

    int mAlpha = 255;

    // Where the image lands: gravity applied to the bounds, or the bounds
    // themselves while tiling.
    Rect mDstRect;
    bool mDstRectDirty = true;
};

} // namespace graphics
} // namespace setu
