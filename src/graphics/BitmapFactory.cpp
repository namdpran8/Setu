#include "BitmapFactory.h"

#include <wincodec.h>
#include <wrl/client.h>

#include <cstdio>
#include <cstring>

#include "androidfw/Asset.h"
#include "androidfw/ResourceTypes.h"

#include "../utils/Logger.h"

namespace setu {
namespace graphics {

namespace {

constexpr const char* kTag = "BitmapFactory";

// Which container the bytes are, decided by magic number rather than by asking WIC.
// Note there is deliberately no GUID_ContainerFormatWebp here: that GUID only
// exists from the Windows 10 1809 SDK, and the format still has to be recognised
// on a machine with no WebP decoder installed at all, so that the failure can be
// explained. A magic-byte sniff answers both.
enum class Container { UNKNOWN, PNG, JPEG, WEBP, GIF, BMP };

const uint8_t kPngSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

// Res_png_9patch::serializedSize() hardcodes the header at 32 bytes, while
// fill9patchOffsets() places the div arrays at sizeof(Res_png_9patch). The two only
// agree if that sizeof really is 32, and under MSVC it is reached by natural layout
// rather than by the __attribute__((packed)) androidfw_compat.h defines away - so
// assert it rather than trust it. If this ever fires, every offset androidfw
// computes into an npTc chunk is wrong, not just this file.
static_assert(sizeof(android::Res_png_9patch) == 32,
              "Res_png_9patch must be 32 bytes to match its own serialized layout");

std::string hrToString(HRESULT hr) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)hr);
    return std::string(buf);
}

// Magic-byte sniff. Deliberately not a guess at the whole container - just enough
// leading bytes to name the format for a log line, pick the mime type, and decide
// whether it is worth walking PNG chunks.
Container sniffContainer(const uint8_t* data, size_t length) {
    if (length >= sizeof(kPngSignature) && memcmp(data, kPngSignature, sizeof(kPngSignature)) == 0) {
        return Container::PNG;
    }
    if (length >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
        return Container::JPEG;
    }
    // RIFF....WEBP - the four-byte size between the two tags is what makes this a
    // 12-byte check rather than an 8-byte one.
    if (length >= 12 && memcmp(data, "RIFF", 4) == 0 && memcmp(data + 8, "WEBP", 4) == 0) {
        return Container::WEBP;
    }
    if (length >= 4 && memcmp(data, "GIF8", 4) == 0) return Container::GIF;
    if (length >= 2 && data[0] == 'B' && data[1] == 'M') return Container::BMP;
    return Container::UNKNOWN;
}

const char* mimeTypeFor(Container container) {
    switch (container) {
        case Container::PNG: return "image/png";
        case Container::JPEG: return "image/jpeg";
        case Container::WEBP: return "image/webp";
        case Container::GIF: return "image/gif";
        case Container::BMP: return "image/bmp";
        default: return "";
    }
}

// WIC is COM, and nothing else in this runtime initialises COM - Direct2D and
// DirectWrite do not need it. So this does, lazily, on whatever thread first
// decodes, and never calls CoUninitialize: the process wants COM for its lifetime,
// and tearing it down while the cached factory below still holds a reference would
// be the actual bug. Safe if WindowManager ever starts initialising COM itself,
// because an already-initialised thread reports S_FALSE or RPC_E_CHANGED_MODE, both
// of which are fine.
bool ensureComInitialized() {
    static thread_local bool attempted = false;
    static thread_local bool initialized = false;
    if (attempted) return initialized;
    attempted = true;

    const HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    // RPC_E_CHANGED_MODE: someone already joined this thread to an STA. COM is
    // usable, just not in the mode asked for.
    initialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;
    if (!initialized) {
        Logger::e(kTag, "CoInitializeEx failed: " + hrToString(hr));
    }
    return initialized;
}

IWICImagingFactory* getWicFactory() {
    static Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    if (factory) return factory.Get();

    const HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                        IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(hr) || !factory) {
        Logger::e(kTag, "CoCreateInstance(WICImagingFactory) failed: " + hrToString(hr));
        factory.Reset();
        return nullptr;
    }
    return factory.Get();
}

// Walks the PNG chunk list looking for "npTc". Structure per the PNG spec, and as
// implemented by androidfw's own PngChunkFilter: an 8-byte signature, then chunks
// of [4-byte big-endian payload length][4-byte type][payload][4-byte CRC].
//
// The validation and normalisation of the payload follow AOSP's
// NinePatchPeeker::readChunk exactly - length must equal serializedSize(), the data
// is copied because the source buffer is not ours, then deserialize() fills the
// offsets and fileToDevice() swaps the payload into host byte order. Only the order
// of the size check and the copy is swapped relative to AOSP, so that
// serializedSize() is read through an aligned pointer instead of straight out of
// the middle of the file buffer.
//
// Returns an empty vector for "no chunk", which is the overwhelmingly common case.
std::vector<uint8_t> extractNinePatchChunk(const uint8_t* data, size_t length) {
    if (length < sizeof(kPngSignature)) return {};

    size_t pos = sizeof(kPngSignature);
    // Length + type + CRC. A chunk header cannot start with fewer bytes left.
    constexpr size_t kChunkOverhead = 12;

    while (pos + kChunkOverhead <= length) {
        const uint32_t payloadLength = ((uint32_t)data[pos] << 24) | ((uint32_t)data[pos + 1] << 16) |
                                       ((uint32_t)data[pos + 2] << 8) | (uint32_t)data[pos + 3];
        const uint8_t* type = data + pos + 4;
        const size_t payloadStart = pos + 8;

        // payloadStart + 4 <= length holds from the loop condition, so this cannot
        // underflow.
        if (payloadLength > length - payloadStart - 4) {
            Logger::w(kTag, "PNG chunk at byte " + std::to_string(pos) + " claims " +
                                std::to_string(payloadLength) + " bytes, past end of file");
            return {};
        }

        if (memcmp(type, "npTc", 4) == 0) {
            if (payloadLength < sizeof(android::Res_png_9patch)) {
                Logger::w(kTag, "npTc chunk is " + std::to_string(payloadLength) +
                                    " bytes, shorter than the header");
                return {};
            }

            // data() of a non-empty vector comes from operator new, so it is
            // aligned for max_align_t - enough for Res_png_9patch's alignas.
            std::vector<uint8_t> chunk(data + payloadStart, data + payloadStart + payloadLength);
            android::Res_png_9patch* patch =
                reinterpret_cast<android::Res_png_9patch*>(chunk.data());

            const size_t serialized = patch->serializedSize();
            if (serialized != payloadLength) {
                Logger::w(kTag, "npTc chunk is " + std::to_string(payloadLength) +
                                    " bytes but describes " + std::to_string(serialized));
                return {};
            }

            android::Res_png_9patch::deserialize(chunk.data());
            patch->fileToDevice();
            return chunk;
        }

        if (memcmp(type, "IEND", 4) == 0) return {};

        pos = payloadStart + payloadLength + 4;
    }
    return {};
}

std::shared_ptr<Bitmap> decodeMemory(const uint8_t* data, size_t length,
                                     BitmapFactory::Options& opts) {
    opts.outWidth = -1;
    opts.outHeight = -1;
    opts.outMimeType.clear();
    opts.outNinePatchChunk.clear();
    opts.outPadding.set(-1, -1, -1, -1);

    if (!data || length == 0) {
        Logger::e(kTag, "decode: empty buffer");
        return nullptr;
    }
    if (length > MAXDWORD) {
        Logger::e(kTag, "decode: " + std::to_string(length) + " bytes exceeds what a WIC memory "
                                                              "stream can address");
        return nullptr;
    }

    const Container container = sniffContainer(data, length);
    opts.outMimeType = mimeTypeFor(container);

    if (container == Container::PNG) {
        opts.outNinePatchChunk = extractNinePatchChunk(data, length);
        if (!opts.outNinePatchChunk.empty()) {
            const android::Res_png_9patch* patch =
                reinterpret_cast<const android::Res_png_9patch*>(opts.outNinePatchChunk.data());
            opts.outPadding.set(patch->paddingLeft, patch->paddingTop, patch->paddingRight,
                                patch->paddingBottom);
        }
    }

    if (!ensureComInitialized()) return nullptr;
    IWICImagingFactory* factory = getWicFactory();
    if (!factory) return nullptr;

    // InitializeFromMemory borrows the buffer rather than copying it, and wants a
    // non-const pointer even though a decode never writes through it. Everything
    // holding that pointer is released before this function returns, which is what
    // keeps the borrow inside the caller's guarantee about the asset.
    Microsoft::WRL::ComPtr<IWICStream> stream;
    HRESULT hr = factory->CreateStream(&stream);
    if (FAILED(hr) || !stream) {
        Logger::e(kTag, "CreateStream failed: " + hrToString(hr));
        return nullptr;
    }
    hr = stream->InitializeFromMemory(const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data)),
                                      (DWORD)length);
    if (FAILED(hr)) {
        Logger::e(kTag, "InitializeFromMemory failed: " + hrToString(hr));
        return nullptr;
    }

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand,
                                          &decoder);
    if (FAILED(hr) || !decoder) {
        if (hr == WINCODEC_ERR_COMPONENTNOTFOUND && container == Container::WEBP) {
            // The failure a real APK is most likely to hit, so it gets a diagnosis
            // instead of an HRESULT. Windows has no built-in WebP WIC decoder.
            Logger::e(kTag, "No WebP decoder installed. Windows ships no built-in WebP WIC codec; "
                            "it comes from the 'Webp Image Extensions' package in the Microsoft "
                            "Store. This image cannot be decoded until that is present.");
        } else if (hr == WINCODEC_ERR_COMPONENTNOTFOUND) {
            Logger::e(kTag, std::string("No WIC decoder installed for ") +
                                (container == Container::UNKNOWN
                                     ? "this image (unrecognised container)"
                                     : mimeTypeFor(container)));
        } else {
            Logger::e(kTag, "CreateDecoderFromStream failed: " + hrToString(hr));
        }
        return nullptr;
    }

    // Frame 0. An animated GIF or WebP decodes to its first frame, which is what
    // BitmapFactory does too - animation is AnimatedImageDrawable's problem.
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr) || !frame) {
        Logger::e(kTag, "GetFrame(0) failed: " + hrToString(hr));
        return nullptr;
    }

    UINT width = 0;
    UINT height = 0;
    hr = frame->GetSize(&width, &height);
    if (FAILED(hr)) {
        Logger::e(kTag, "GetSize failed: " + hrToString(hr));
        return nullptr;
    }
    opts.outWidth = (int)width;
    opts.outHeight = (int)height;

    if (opts.inJustDecodeBounds) return nullptr;

    // 32bppPBGRA is Bitmap's storage format, so this converter is the whole of the
    // pixel pipeline: it handles palette expansion, 16-bit reduction, greyscale,
    // CMYK JPEG and un-premultiplied sources, and lands on exactly the bytes the
    // Bitmap wants.
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr) || !converter) {
        Logger::e(kTag, "CreateFormatConverter failed: " + hrToString(hr));
        return nullptr;
    }
    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0f,
                               WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) {
        Logger::e(kTag, "Converter Initialize to 32bppPBGRA failed: " + hrToString(hr));
        return nullptr;
    }

    std::shared_ptr<Bitmap> bitmap = Bitmap::createBitmap((int)width, (int)height, opts.inDensity);
    if (!bitmap) return nullptr; // createBitmap has logged why

    uint32_t* dest = bitmap->getRowForWrite(0);
    if (!dest) return nullptr;

    // Both casts are bounded by Bitmap's own allocation cap, which is far below
    // UINT_MAX.
    hr = converter->CopyPixels(nullptr, (UINT)bitmap->getRowBytes(), (UINT)bitmap->getByteCount(),
                               reinterpret_cast<BYTE*>(dest));
    if (FAILED(hr)) {
        Logger::e(kTag, "CopyPixels failed: " + hrToString(hr));
        return nullptr;
    }

    Logger::d(kTag, "Decoded " + std::to_string(width) + "x" + std::to_string(height) + " " +
                        (opts.outMimeType.empty() ? "image" : opts.outMimeType) +
                        (opts.outNinePatchChunk.empty() ? "" : " (nine-patch)") + " at density " +
                        std::to_string(opts.inDensity));
    return bitmap;
}

} // namespace

std::shared_ptr<Bitmap> BitmapFactory::decodeAsset(android::Asset* asset, Options* options) {
    Options scratch;
    Options& opts = options ? *options : scratch;

    if (!asset) {
        Logger::e(kTag, "decodeAsset: null asset");
        return nullptr;
    }

    const off64_t length = asset->getLength();
    if (length <= 0) {
        Logger::e(kTag, "decodeAsset: asset length " + std::to_string((long long)length));
        return nullptr;
    }

    // aligned = false: nothing here reinterprets the asset buffer as a wider type
    // in place. The 9-patch chunk is copied into an aligned vector before it is
    // cast, and the pixels go through WIC.
    const void* buffer = asset->getBuffer(false);
    if (!buffer) {
        Logger::e(kTag, "decodeAsset: getBuffer returned null");
        return nullptr;
    }

    return decodeMemory(reinterpret_cast<const uint8_t*>(buffer), (size_t)length, opts);
}

std::shared_ptr<Bitmap> BitmapFactory::decodeByteArray(const void* data, size_t length,
                                                       Options* options) {
    Options scratch;
    Options& opts = options ? *options : scratch;
    return decodeMemory(reinterpret_cast<const uint8_t*>(data), length, opts);
}

} // namespace graphics
} // namespace setu
