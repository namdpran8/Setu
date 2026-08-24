#pragma once

#include <memory>
#include <string>

#include "Paint.h"
#include "TextLayout.h"

// Forward-declared so this header stays free of <dwrite.h>; it is included from
// both the view layer and the canvas layer.
struct IDWriteFactory;
struct IDWriteTextFormat;

namespace setu {
namespace graphics {

// The single place in Windroid that knows which font we render with and how a
// string turns into glyph metrics.
//
// Before this existed, TextView::onMeasure and Direct2DCanvas::drawText each
// created their own IDWriteTextFormat with their own hardcoded family name and
// their own layout width, so the size a view reserved and the size the text
// actually drew at were only coincidentally the same. Everything now goes
// through here, which means they cannot drift apart.
//
// The family name is deliberately still a Windows font. Real Roboto parity
// needs a font file shipped with the runtime; when that lands, setFamilyName()
// (or the mFamilyName default) is the only thing that has to change.
class FontManager {
public:
    // Layout width meaning "do not wrap". Large enough that no real string
    // reaches it, small enough to stay well inside float precision.
    static constexpr float UNBOUNDED_WIDTH = 100000.0f;

    static FontManager& getInstance();

    // Called once by WindowManager after DirectWrite comes up. Passing a
    // different factory drops every cached format, since formats are bound to
    // the factory that created them.
    void setFactory(IDWriteFactory* factory);
    IDWriteFactory* getFactory() const { return mFactory; }

    const wchar_t* getFamilyName() const { return mFamilyName; }
    void setFamilyName(const wchar_t* family);

    // Shared, cached format for this paint's text size. Borrowed, not owned:
    // do not Release it.
    IDWriteTextFormat* getTextFormat(const Paint& paint);

    // Line-breaks `text` against `maxWidth` and returns the resulting layout.
    // Pass UNBOUNDED_WIDTH (or anything <= 0) for a single unwrapped line.
    std::shared_ptr<TextLayout> getTextLayout(const std::wstring& text, const Paint& paint,
                                              float maxWidth);

    // Advance width of a single unwrapped run, trailing whitespace included.
    float measureText(const std::wstring& text, const Paint& paint);

    // Font-wide vertical metrics at this paint's text size. Cached per size.
    FontMetrics getFontMetrics(const Paint& paint);

private:
    FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // Fills in `layout`'s line list without DirectWrite, by greedily wrapping on
    // spaces using GDI measurements. Not pixel-accurate, but it keeps measure
    // and draw consistent with each other when DirectWrite is unavailable.
    void wrapWithFallback(TextLayout& layout, const Paint& paint, float maxWidth);

    // Not owned; DirectWrite factory lives in WindowManager.
    IDWriteFactory* mFactory = nullptr;

    // TODO(font-parity): ship Roboto and point this at it. Segoe UI is the
    // closest thing Windows guarantees is installed, but its metrics are not
    // Roboto's, so text will not be pixel-identical to a device until then.
    const wchar_t* mFamilyName = L"Segoe UI";
};

} // namespace graphics
} // namespace setu
