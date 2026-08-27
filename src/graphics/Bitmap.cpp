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

#include "Bitmap.h"

#include <d2d1_1.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <string>

#include "../utils/Logger.h"

namespace setu {
namespace graphics {

namespace {

constexpr const char* kTag = "Bitmap";

// A ceiling on a single allocation, so that a corrupt image header asking for
// 60000x60000 is refused instead of attempting 14GB. Android's own limit is far
// looser (it fails when the heap fails); this is deliberately tighter, because
// nothing a layout draws is legitimately this big.
constexpr size_t kMaxByteCount = 256u * 1024u * 1024u;

std::string hrToString(HRESULT hr) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)hr);
    return std::string(buf);
}

// Skia's SkMulDiv255Round: (value * alpha) / 255, rounded, without a divide.
inline uint32_t mulDiv255Round(uint32_t value, uint32_t alpha) {
    const uint32_t prod = value * alpha + 128;
    return (prod + (prod >> 8)) >> 8;
}

} // namespace

// Defined here rather than in the header so that ID2D1Bitmap stays opaque to
// everything that merely holds a Bitmap.
struct Bitmap::D2DCache {
    struct Entry {
        // Borrowed, not AddRef'd - see the getD2DBitmap comment in the header for
        // why, and for the invalidation rule that choice imposes on the caller.
        ID2D1RenderTarget* target = nullptr;
        Microsoft::WRL::ComPtr<ID2D1Bitmap> bitmap;
        uint32_t generation = 0;
    };
    std::vector<Entry> entries;
};

Bitmap::Bitmap(int width, int height, size_t strideBytes, int density)
    : mWidth(width),
      mHeight(height),
      mStride(strideBytes),
      mRowWords(strideBytes / BYTES_PER_PIXEL),
      mDensity(density) {}

Bitmap::~Bitmap() = default;

bool Bitmap::validateGeometry(const char* what, int width, int height, size_t strideBytes) {
    if (width <= 0 || height <= 0) {
        Logger::e(kTag, std::string(what) + ": bad size " + std::to_string(width) + "x" +
                            std::to_string(height));
        return false;
    }
    const size_t tight = (size_t)width * BYTES_PER_PIXEL;
    if (strideBytes < tight) {
        Logger::e(kTag, std::string(what) + ": stride " + std::to_string(strideBytes) +
                            " is narrower than " + std::to_string(width) + " pixels");
        return false;
    }
    // Every 8888 decoder produces a 4-byte-aligned stride. Rejecting anything else
    // keeps the store a uint32_t vector, which is what guarantees rows are aligned
    // for a 32-bit load.
    if (strideBytes % BYTES_PER_PIXEL != 0) {
        Logger::e(kTag, std::string(what) + ": stride " + std::to_string(strideBytes) +
                            " is not a multiple of 4");
        return false;
    }
    if (strideBytes > kMaxByteCount / (size_t)height) {
        Logger::e(kTag, std::string(what) + ": " + std::to_string(width) + "x" +
                            std::to_string(height) + " at stride " + std::to_string(strideBytes) +
                            " exceeds the " + std::to_string(kMaxByteCount / (1024 * 1024)) +
                            "MB cap");
        return false;
    }
    return true;
}

std::shared_ptr<Bitmap> Bitmap::createBitmap(int width, int height, int density) {
    const size_t stride = (size_t)(width > 0 ? width : 0) * BYTES_PER_PIXEL;
    if (!validateGeometry("createBitmap", width, height, stride)) return nullptr;

    // new, not make_shared: the constructor is private so that every Bitmap comes
    // from a factory that has validated its geometry.
    std::shared_ptr<Bitmap> bitmap(new Bitmap(width, height, stride, density));
    bitmap->mPixels.assign(bitmap->mRowWords * (size_t)height, 0u);
    return bitmap;
}

std::shared_ptr<Bitmap> Bitmap::wrapPixels(int width, int height, size_t strideBytes,
                                           std::vector<uint32_t>&& premultipliedPixels,
                                           int density) {
    if (strideBytes == 0) strideBytes = (size_t)(width > 0 ? width : 0) * BYTES_PER_PIXEL;
    if (!validateGeometry("wrapPixels", width, height, strideBytes)) return nullptr;

    const size_t needed = (strideBytes / BYTES_PER_PIXEL) * (size_t)height;
    if (premultipliedPixels.size() < needed) {
        Logger::e(kTag, "wrapPixels: buffer holds " + std::to_string(premultipliedPixels.size()) +
                            " words, needs " + std::to_string(needed));
        return nullptr;
    }

    std::shared_ptr<Bitmap> bitmap(new Bitmap(width, height, strideBytes, density));
    bitmap->mPixels = std::move(premultipliedPixels);
    return bitmap;
}

uint32_t Bitmap::premultiply(uint32_t color) {
    const uint32_t a = color >> 24;
    if (a == 255) return color;
    if (a == 0) return 0;
    const uint32_t r = mulDiv255Round((color >> 16) & 0xFF, a);
    const uint32_t g = mulDiv255Round((color >> 8) & 0xFF, a);
    const uint32_t b = mulDiv255Round(color & 0xFF, a);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t Bitmap::unpremultiply(uint32_t premultiplied) {
    const uint32_t a = premultiplied >> 24;
    if (a == 255) return premultiplied;
    if (a == 0) return 0;
    // Clamped because a channel above its own alpha is representable but not
    // valid, and malformed source data does contain it.
    const auto expand = [a](uint32_t c) {
        const uint32_t v = (c * 255u + (a >> 1)) / a;
        return v > 255u ? 255u : v;
    };
    const uint32_t r = expand((premultiplied >> 16) & 0xFF);
    const uint32_t g = expand((premultiplied >> 8) & 0xFF);
    const uint32_t b = expand(premultiplied & 0xFF);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

int Bitmap::scaleFromDensity(int size, int sourceDensity, int targetDensity) {
    if (sourceDensity == DENSITY_NONE || targetDensity == DENSITY_NONE ||
        sourceDensity == targetDensity) {
        return size;
    }
    return ((size * targetDensity) + (sourceDensity >> 1)) / sourceDensity;
}

const uint32_t* Bitmap::getRow(int y) const {
    if (y < 0 || y >= mHeight) return nullptr;
    return mPixels.data() + mRowWords * (size_t)y;
}

uint32_t* Bitmap::getRowForWrite(int y) {
    if (y < 0 || y >= mHeight) return nullptr;
    ++mGeneration;
    return mPixels.data() + mRowWords * (size_t)y;
}

uint32_t Bitmap::getPixel(int x, int y) const {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return 0;
    return unpremultiply(mPixels[mRowWords * (size_t)y + (size_t)x]);
}

void Bitmap::setPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= mWidth || y < 0 || y >= mHeight) return;
    mPixels[mRowWords * (size_t)y + (size_t)x] = premultiply(color);
    ++mGeneration;
}

void Bitmap::eraseColor(uint32_t color) {
    const uint32_t pm = premultiply(color);
    if (mRowWords == (size_t)mWidth) {
        // Tightly packed: one fill over the whole store.
        std::fill(mPixels.begin(), mPixels.begin() + mRowWords * (size_t)mHeight, pm);
    } else {
        for (int y = 0; y < mHeight; ++y) {
            uint32_t* row = mPixels.data() + mRowWords * (size_t)y;
            std::fill(row, row + mWidth, pm);
        }
    }
    ++mGeneration;
}

ID2D1Bitmap* Bitmap::getD2DBitmap(ID2D1RenderTarget* target) const {
    if (!target || mPixels.empty()) return nullptr;

    if (!mD2DCache) mD2DCache = std::make_unique<D2DCache>();

    D2DCache::Entry* entry = nullptr;
    for (D2DCache::Entry& candidate : mD2DCache->entries) {
        if (candidate.target == target) {
            entry = &candidate;
            break;
        }
    }

    if (entry && entry->bitmap) {
        if (entry->generation == mGeneration) return entry->bitmap.Get();

        // Same dimensions and format, so the pixels can be pushed into the
        // existing texture instead of building a new one.
        HRESULT hr = entry->bitmap->CopyFromMemory(nullptr, mPixels.data(), (UINT32)mStride);
        if (SUCCEEDED(hr)) {
            entry->generation = mGeneration;
            return entry->bitmap.Get();
        }
        Logger::w(kTag, "CopyFromMemory failed (" + hrToString(hr) + "), recreating");
        entry->bitmap.Reset();
    }

    D2D1_BITMAP_PROPERTIES props;
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    // Pinned to 96, not left at the target's DPI. The view tree lays out in device
    // pixels already, so a D2D DIP has to stay 1:1 with a bitmap pixel; inheriting
    // a 192-DPI target here would scale every image a second time.
    props.dpiX = 96.0f;
    props.dpiY = 96.0f;

    Microsoft::WRL::ComPtr<ID2D1Bitmap> created;
    HRESULT hr = target->CreateBitmap(D2D1::SizeU((UINT32)mWidth, (UINT32)mHeight), mPixels.data(),
                                      (UINT32)mStride, &props, &created);
    if (FAILED(hr) || !created) {
        Logger::e(kTag, "CreateBitmap failed for " + std::to_string(mWidth) + "x" +
                            std::to_string(mHeight) + ": " + hrToString(hr));
        return nullptr;
    }

    if (!entry) {
        mD2DCache->entries.emplace_back();
        entry = &mD2DCache->entries.back();
        entry->target = target;
    }
    entry->bitmap = created;
    entry->generation = mGeneration;
    return entry->bitmap.Get();
}

void Bitmap::releaseD2DBitmap(ID2D1RenderTarget* target) {
    if (!mD2DCache) return;
    std::vector<D2DCache::Entry>& entries = mD2DCache->entries;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].target != target) continue;
        entries.erase(entries.begin() + (ptrdiff_t)i);
        return;
    }
}

void Bitmap::invalidateD2DBitmaps() {
    if (mD2DCache) mD2DCache->entries.clear();
}

} // namespace graphics
} // namespace setu
