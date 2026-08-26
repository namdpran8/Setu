#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "Drawable.h"
#include "../Bitmap.h"

namespace setu {
namespace graphics {

class Canvas;

// android.graphics.drawable.NinePatchDrawable: a bitmap with a border-encoded
// description of which parts of it may stretch.
//
// This is the drawable that makes stock Android chrome resizable. A button
// background is one small PNG whose corners must stay pixel-crisp at any width
// while the middle stretches, and whose black border marks also declare where the
// *text* is allowed to sit. Both halves of that matter here, and the second one is
// the reason this class earns its keep:
//
//   * The stretch regions come from the npTc chunk's divs and drive draw(), which
//     blits the image as a grid rather than as one stretched rectangle.
//
//   * The padding rect from the same chunk is reported through getPadding(), which
//     View::setBackground already turns into the view's padding. That is what makes
//     a 9-patch button size its label correctly instead of printing it flush
//     against the artwork's edge.
//
// AOSP splits this across three objects - NinePatchDrawable holds a NinePatch,
// which holds a Bitmap and the chunk, and the grid walk itself lives down in native
// Skia (SkLatticeIter). There is no NinePatch class here and no Skia, so this class
// is all three: it parses the chunk, computes the lattice, and issues the draws.
//
// The chunk is expected in *deserialized* form - offsets filled in by
// Res_png_9patch::deserialize and the payload byte-swapped into host order by
// fileToDevice. That is exactly what BitmapFactory::Options::outNinePatchChunk
// hands back, and it is the only supported source: a chunk that still has
// wasDeserialized clear is rejected rather than guessed at, because the byte-order
// conversion is not idempotent and applying it twice would silently produce
// plausible-looking garbage divs.
//
// A chunk that fails validation is not fatal. The drawable falls back to a single
// stretched cell - which is what the image would have looked like as a plain
// BitmapDrawable - and logs why. A button with slightly wrong corners beats an
// invisible one.
//
// Deliberately absent: optical insets (AOSP's mOpticalInsets and getOpticalInsets,
// which Drawable here has no hook to report through), the outline/radius the chunk
// can also carry (that feeds shadow casting, which does not exist yet), tinting,
// android:dither, and autoMirrored - this runtime is LTR-only.
class NinePatchDrawable : public Drawable {
public:
    NinePatchDrawable() = default;

    // `ninePatchChunk` is borrowed, not retained: everything needed is copied out of
    // it during construction, so the caller's vector can go out of scope
    // immediately. That also keeps this class free of the pointer arithmetic
    // Res_png_9patch does relative to its own address.
    NinePatchDrawable(std::shared_ptr<const Bitmap> bitmap,
                      const std::vector<uint8_t>& ninePatchChunk);

    void draw(Canvas& canvas) override;

    // Returns false when the chunk could not be used, having logged the reason. The
    // drawable is still usable in that case - it just stretches like a plain bitmap.
    bool setNinePatch(std::shared_ptr<const Bitmap> bitmap,
                      const std::vector<uint8_t>& ninePatchChunk);

    const std::shared_ptr<const Bitmap>& getBitmap() const { return mBitmap; }

    // False when the chunk was rejected and the single-stretched-cell fallback is in
    // effect. Worth checking at inflate time, where a plain BitmapDrawable would be
    // the more honest thing to build instead.
    bool hasValidChunk() const { return mChunkValid; }

    // Screen density in DPI - 480 for xxhdpi, not WindowManager's 3.0 scale factor.
    // Scales the intrinsic size, the reported padding, and the fixed cells of the
    // grid, all three of which AOSP recomputes together in computeBitmapSize().
    void setTargetDensity(int densityDpi);
    int getTargetDensity() const { return mTargetDensity; }

    int getIntrinsicWidth() const override { return mBitmapWidth; }
    int getIntrinsicHeight() const override { return mBitmapHeight; }

    // The chunk's padding rect, density-scaled. This is the hook View::setBackground
    // already calls: a nine-patch background with padding sets the view's padding
    // unless the layout gave it one explicitly.
    //
    // Returns false when all four edges are zero, matching AOSP's
    // `(left | top | right | bottom) != 0` and this codebase's Drawable contract -
    // which is what stops a paddingless background from wiping out the chrome insets
    // a widget like Button set for itself.
    bool getPadding(Rect& padding) const override;

    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

protected:
    void onBoundsChange(const Rect& bounds) override;

private:
    // One cell extent along one axis: where it comes from in the bitmap, and where
    // it lands on the canvas. Kept as floats on the destination side because a
    // stretched cell rarely divides evenly, and carrying the fractional position
    // forward is what keeps adjacent cells sharing an exact edge instead of leaving
    // a hairline seam between them.
    struct Span {
        float srcLo;
        float srcHi;
        float dstLo;
        float dstHi;
    };

    // AOSP's computeBitmapSize: intrinsic size, scaled padding and the density ratio
    // the grid is built with, all rederived together.
    void computeBitmapSize();

    // Copies divs, cell colours and the padding rect out of the chunk, validating as
    // it goes. Returns false and leaves the parsed state empty on any inconsistency.
    bool parseChunk(const std::vector<uint8_t>& chunk);

    // Rebuilds mCols/mRows for the current bounds. Cheap, but bounds-driven rather
    // than per-draw, the way Drawable asks subclasses to rebuild cached geometry.
    void updateLatticeIfDirty();

    // Lays one axis out. A member only because Span is private; it touches no state
    // beyond its arguments, which is what keeps the two axes provably independent.
    static void buildSpans(const std::vector<int32_t>& divs, int sourceSize,
                           float densityScale, float dstLo, float dstHi,
                           std::vector<Span>& out);

    std::shared_ptr<const Bitmap> mBitmap;

    // Division boundaries in source pixels, copied out of the chunk. Column i spans
    // [div[i-1], div[i]) with the image edges standing in at either end, and
    // odd-numbered columns are the stretchable ones - see updateLatticeIfDirty.
    std::vector<int32_t> mXDivs;
    std::vector<int32_t> mYDivs;

    // One entry per grid cell, row-major. Empty when the chunk's count did not match
    // the grid, in which case the transparent-cell skip is simply not applied.
    std::vector<uint32_t> mCellColors;

    bool mChunkValid = false;

    int mTargetDensity = Bitmap::DENSITY_DEFAULT;

    // Destination pixels per source pixel, from the density ratio. 1.0 whenever the
    // asset was authored for this screen, which is the case that has to stay exact:
    // it keeps every fixed cell on an integer boundary, and therefore keeps the
    // corners from being resampled.
    float mDensityScale = 1.0f;

    int mBitmapWidth = -1;
    int mBitmapHeight = -1;

    Rect mSourcePadding;  // as authored, in source pixels
    Rect mPadding;        // mSourcePadding scaled to mTargetDensity

    int mAlpha = 255;

    std::vector<Span> mCols;
    std::vector<Span> mRows;
    bool mLatticeDirty = true;
};

} // namespace graphics
} // namespace setu
