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

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// Forward-declared instead of including <d2d1.h>. A Bitmap is going to be held by
// BitmapDrawable, ImageView, and every display list that draws one; none of those
// want the Direct2D headers pulled in behind them. The one member that genuinely
// needs D2D types is the per-render-target cache, and it lives behind an opaque
// pointer whose definition is in Bitmap.cpp.
struct ID2D1Bitmap;
struct ID2D1RenderTarget;

namespace setu {
namespace graphics {

// android.graphics.Bitmap: a fixed-size block of decoded pixels, plus the screen
// density they were authored for.
//
// Exactly one pixel format, ARGB_8888 premultiplied, because that is the only one
// the rest of Phase 6 needs and every extra config costs a branch in every
// consumer. Two consequences worth stating up front:
//
//   * Pixels are uint32_t in 0xAARRGGBB order - the same Color int the runtime
//     already passes around (Paint::getColor, Canvas::drawColor). On a
//     little-endian machine that puts B at the lowest address, so the buffer is
//     bit-for-bit DXGI_FORMAT_B8G8R8A8_UNORM and uploads to Direct2D with no
//     swizzle pass. That equivalence is little-endian-only, which is the only
//     thing this runtime targets.
//
//   * The store is premultiplied, matching both Skia's kPremul_SkAlphaType and
//     the D2D1_ALPHA_MODE_PREMULTIPLIED the upload declares. getPixel/setPixel
//     un- and re-premultiply on the way through so that they keep Android's
//     non-premultiplied contract; the row accessors do not, and hand back the
//     raw store for decoders and backends.
//
// Not a value type. Android Bitmaps are shared, reference-counted objects handed
// around by reference, and copying one here would have to duplicate a megabyte of
// pixels and decide what to do with the GPU cache. So: private constructor, static
// factories returning shared_ptr, copying deleted.
//
// Deferred deliberately, so a later phase does not have to guess whether it was
// forgotten: mutability tracking (Android's isMutable), Bitmap::reconfigure,
// setHasAlpha's opaque fast path (an opaque image renders correctly through
// PREMULTIPLIED anyway - the only cost is that D2D cannot skip the blend), and
// non-premultiplied ARGB_8888 (Android's setPremultiplied(false), which only
// exists for pixel-copy interop and never for drawing).
//
// enable_shared_from_this is what lets RecordingCanvas turn the const Bitmap& in
// Canvas::drawBitmap into a shared_ptr<const Bitmap> for the display list, so a
// recorded draw keeps the image alive without copying its pixels. It cannot fail:
// the constructor is private and every factory returns a shared_ptr, so an unowned
// Bitmap does not exist.
class Bitmap : public std::enable_shared_from_this<Bitmap> {
public:
    // Density values are DPI, as in DisplayMetrics: 160 is the baseline where
    // 1dp == 1px. DENSITY_NONE means "unspecified", and turns density scaling off
    // rather than scaling by zero.
    //
    // Careful: the density this class stores is NOT the same unit as
    // WindowManager::getDensity(), which is a float scale factor (2.0 for an
    // xhdpi screen). Converting is dpi = round(scale * DENSITY_DEFAULT). Mixing
    // the two silently scales by 160x, so callers should convert at the boundary.
    static constexpr int DENSITY_NONE = 0;
    static constexpr int DENSITY_DEFAULT = 160;

    static constexpr size_t BYTES_PER_PIXEL = 4;

    // Allocates a zero-filled (fully transparent) bitmap. Returns nullptr, with a
    // log line, on a degenerate or absurd size rather than throwing: a bad image
    // header should cost one missing drawable, not the process.
    static std::shared_ptr<Bitmap> createBitmap(int width, int height,
                                                int density = DENSITY_NONE);

    // Takes ownership of an already-decoded, already-premultiplied buffer. This is
    // the entry point a decoder uses; strideBytes may exceed width * 4 because
    // most imaging APIs pad rows, and 0 means "tightly packed".
    static std::shared_ptr<Bitmap> wrapPixels(int width, int height, size_t strideBytes,
                                              std::vector<uint32_t>&& premultipliedPixels,
                                              int density = DENSITY_NONE);

    ~Bitmap();

    Bitmap(const Bitmap&) = delete;
    Bitmap& operator=(const Bitmap&) = delete;

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }

    // The stride, in bytes. Android's spelling of it, and the pitch a backend
    // hands to the GPU. Always a multiple of 4 and always >= width * 4.
    size_t getRowBytes() const { return mStride; }
    size_t getByteCount() const { return mStride * (size_t)mHeight; }

    // The density the pixels were authored at, e.g. 240 for a drawable-hdpi asset.
    // A later BitmapFactory is what will set this from the resource directory it
    // found the file in; a bitmap built in memory has no inherent density, hence
    // the DENSITY_NONE default.
    int getDensity() const { return mDensity; }
    void setDensity(int density) { mDensity = density; }

    // Size this bitmap should draw at on a targetDensity screen. This is the whole
    // reason the density field exists.
    int getScaledWidth(int targetDensity) const {
        return scaleFromDensity(mWidth, mDensity, targetDensity);
    }
    int getScaledHeight(int targetDensity) const {
        return scaleFromDensity(mHeight, mDensity, targetDensity);
    }

    // Non-premultiplied 0xAARRGGBB, matching Bitmap.getPixel/setPixel. Out-of-
    // bounds reads return 0 and writes are dropped; Android throws, but there is
    // no exception path to throw into here.
    uint32_t getPixel(int x, int y) const;
    void setPixel(int x, int y, uint32_t color);

    // Fills every visible pixel with color (non-premultiplied in, like
    // Bitmap.eraseColor). Row padding is left alone - it is not part of the image.
    void eraseColor(uint32_t color);

    // Raw premultiplied row access, for decoders filling the buffer and backends
    // reading it. nullptr if y is out of range. Each row is getWidth() valid
    // words followed by (getRowBytes() / 4 - getWidth()) words of padding.
    const uint32_t* getRow(int y) const;

    // Same, for writing. Marks the GPU cache stale, so a decoder that writes rows
    // and then draws does not need to remember to call notifyPixelsChanged.
    uint32_t* getRowForWrite(int y);

    // Base of the store. Premultiplied, getRowBytes()-pitched.
    const uint32_t* getPixels() const { return mPixels.data(); }

    // Call after mutating pixels through anything other than the accessors above.
    // The next getD2DBitmap re-uploads instead of handing back a stale texture.
    void notifyPixelsChanged() { ++mGeneration; }

    // The cached device bitmap for this render target, created on first use and
    // re-uploaded when the pixels have changed since the last upload. Keyed per
    // target because an ID2D1Bitmap belongs to the target that created it and
    // cannot be drawn by another; in practice this runtime has exactly one device
    // context, so the cache holds one entry and a linear scan is the right lookup.
    //
    // The key is a borrowed, un-AddRef'd pointer - keeping a reference here would
    // pin the swap chain's target alive for as long as any bitmap existed. The
    // cost of that choice is the one rule this class cannot enforce itself:
    // whoever recreates the device context (device loss, resize) must call
    // invalidateD2DBitmaps, because a new context can land on the freed address of
    // the old one and quietly match a dead cache entry.
    //
    // Returns nullptr on failure, having logged; a caller should skip the draw
    // rather than treat it as fatal.
    //
    // const, with the cache mutable behind it, because uploading to the GPU does
    // not change the image - two calls hand back the same pixels. That is what lets
    // a canvas draw from a const Bitmap& and a display list hold a
    // shared_ptr<const Bitmap>, with no const_cast anywhere on the draw path.
    //
    // The caveat that buys: this is not the thread-safe kind of const. Two threads
    // calling getD2DBitmap on one Bitmap would race on the cache vector. Drawing
    // here is single-threaded, and a lock would cost every draw call to protect
    // against a caller that does not exist.
    ID2D1Bitmap* getD2DBitmap(ID2D1RenderTarget* target) const;

    // Drop one target's cached bitmap, or all of them. Non-const, unlike the
    // accessor above: this is deliberate, so that discarding device resources reads
    // as something done *to* the bitmap rather than something incidental.
    void releaseD2DBitmap(ID2D1RenderTarget* target);
    void invalidateD2DBitmaps();

    // 0xAARRGGBB <-> premultiplied 0xAARRGGBB. premultiply uses Skia's
    // SkMulDiv255Round so that a value round-tripped through Skia and through this
    // lands on the same byte.
    static uint32_t premultiply(uint32_t color);

    // Lossy in the small-alpha direction, unavoidably: alpha 2 leaves 128 distinct
    // channel values, and the other 127 were destroyed at premultiply time. Skia
    // has the same hole and papers over it with a rounding table; this rounds
    // directly, which differs from that table by at most one unit.
    static uint32_t unpremultiply(uint32_t premultiplied);

private:
    Bitmap(int width, int height, size_t strideBytes, int density);

    // Bitmap.scaleFromDensity. AOSP's comment there claims it rounds up; the
    // arithmetic rounds to nearest. The arithmetic is what ships on devices, so
    // the arithmetic is what is copied.
    static int scaleFromDensity(int size, int sourceDensity, int targetDensity);

    static bool validateGeometry(const char* what, int width, int height, size_t strideBytes);

    int mWidth = 0;
    int mHeight = 0;
    size_t mStride = 0;     // bytes per row, >= mWidth * 4, multiple of 4
    size_t mRowWords = 0;   // mStride / 4, so row math stays in uint32_t units
    int mDensity = DENSITY_NONE;

    std::vector<uint32_t> mPixels; // premultiplied 0xAARRGGBB, mRowWords per row

    // Bumped on every mutation. A cache entry records the value it uploaded, which
    // is what makes "have the pixels changed since?" a single integer compare.
    uint32_t mGeneration = 1;

    struct D2DCache;
    // mutable: a GPU upload is a cache fill, not a change to the image, so
    // getD2DBitmap is const. Allocated on first getD2DBitmap.
    mutable std::unique_ptr<D2DCache> mD2DCache;
};

} // namespace graphics
} // namespace setu
