#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Bitmap.h"
#include "Rect.h"

namespace android {
class Asset;
}

namespace setu {
namespace graphics {

// android.graphics.BitmapFactory: turns the encoded bytes of a resource into a
// Bitmap. Decoding is Windows Imaging Component, so the format list is whatever
// WIC has a codec for rather than whatever Skia was built with.
//
// The one conversion that matters is the target pixel format:
// GUID_WICPixelFormat32bppPBGRA is premultiplied BGRA, which is bit-for-bit what
// Bitmap stores. So the decode lands straight in the Bitmap's buffer through a
// single CopyPixels - no swizzle, no premultiply pass, no intermediate copy.
//
// WEBP IS NOT A BUILT-IN WINDOWS CODEC. WIC has a predefined container GUID for
// it (GUID_ContainerFormatWebp) and ships WebP *metadata* readers, but there is no
// CLSID_WICWebpDecoder in the built-in decoder list - the decoder arrives with the
// "Webp Image Extensions" package from the Microsoft Store. On a machine without
// it, CreateDecoderFromStream returns WINCODEC_ERR_COMPONENTNOTFOUND and this
// class logs that specific diagnosis rather than a bare HRESULT, because modern
// APKs ship a lot of .webp and "image did not load" is otherwise unexplainable.
// Nothing here can fix that; a bundled libwebp is the fix, and it is not in scope.
//
// Deliberately absent, all of it Options fields that AOSP resamples with:
// inSampleSize, inScaled, inTargetDensity, inPreferredConfig, inBitmap reuse. This
// factory never resamples - it decodes at native pixel size and hands the source
// density to the Bitmap, which is precisely the inScaled=false path in AOSP. The
// scaling then happens at draw time, where BitmapDrawable asks the Bitmap for
// getScaledWidth(targetDensity). One resample fewer, and no quality loss from
// decoding through an intermediate size.
//
// Also absent, matching Android rather than diverging from it: no EXIF
// auto-rotation. BitmapFactory does not rotate; ExifInterface is what reads the
// orientation tag, and a caller that wants it applies it itself.
class BitmapFactory {
public:
    struct Options {
        // ---- in ----

        // The density the encoded bytes were authored at, in DPI - 240 for a file
        // found in res/drawable-hdpi. The resolved value goes onto the returned
        // Bitmap unchanged, which is AOSP's setDensityFromOptions behaviour for an
        // unscaled decode. DENSITY_NONE means "unknown", and disables the density
        // scaling a drawable would otherwise apply.
        int inDensity = Bitmap::DENSITY_NONE;

        // Fill in the out fields and return nullptr without decoding pixels. Same
        // contract as Options.inJustDecodeBounds, including the null return.
        bool inJustDecodeBounds = false;

        // ---- out ----

        // Pixel dimensions of the encoded image, or -1 when they could not be
        // read. Set even when the decode itself fails after this point.
        int outWidth = -1;
        int outHeight = -1;

        // "image/png", "image/webp", ... Empty when the bytes match no container
        // this class recognises. Taken from a magic-byte sniff rather than from
        // the decoder, which gives the same answer for every format that matters
        // and also survives a decode that never got a decoder at all.
        std::string outMimeType;

        // The PNG "npTc" chunk, empty when the image has none. Layout is
        // androidfw's Res_png_9patch (ResourceTypes.h), already normalised the way
        // AOSP's NinePatchPeeker normalises it: deserialize() has filled the div
        // and colour offsets, fileToDevice() has byte-swapped the payload out of
        // PNG's big-endian file order, and wasDeserialized is therefore set - which
        // is the flag Android's own isNinePatchChunk test looks at. Cast the data()
        // pointer straight to Res_png_9patch* and call getXDivs()/getYDivs().
        //
        // Only ever populated for PNG. WIC discards unknown ancillary chunks, so
        // this is harvested by walking the encoded bytes directly - which is also
        // why it has to happen here, at decode time, and cannot be recovered later
        // from the Bitmap.
        std::vector<uint8_t> outNinePatchChunk;

        // The nine-patch content padding, or (-1, -1, -1, -1) when the image has no
        // npTc chunk. Mirrors the outPadding Rect that BitmapFactory.decodeAsset
        // fills from the same four fields.
        Rect outPadding{-1, -1, -1, -1};
    };

    // Decodes an asset opened through AssetManager2::OpenNonAsset. The asset is
    // borrowed for the duration of the call only: pixels are copied out, and the
    // 9-patch chunk is copied into Options, so the caller may close it on return.
    static std::shared_ptr<Bitmap> decodeAsset(android::Asset* asset, Options* options = nullptr);

    // The same decode over a caller-owned buffer, for bytes that did not come from
    // an Asset. Nothing is written through data; the const_cast WIC's memory stream
    // requires is confined to the implementation.
    static std::shared_ptr<Bitmap> decodeByteArray(const void* data, size_t length,
                                                   Options* options = nullptr);
};

} // namespace graphics
} // namespace setu
