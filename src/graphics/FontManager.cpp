#include "FontManager.h"

#include <windows.h>

#include <dwrite_1.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <unordered_map>
#include <vector>

#include "../utils/Logger.h"

using Microsoft::WRL::ComPtr;

namespace setu {
namespace graphics {

namespace {

// Text sizes come from dimension resources and can be fractional, so quantise
// to 1/100 px before using them as a cache key.
uint64_t sizeKey(const Paint& paint) {
    return (uint64_t)(long long)std::lround(paint.getTextSize() * 100.0);
}

std::unordered_map<uint64_t, ComPtr<IDWriteTextFormat>>& formatCache() {
    static std::unordered_map<uint64_t, ComPtr<IDWriteTextFormat>> cache;
    return cache;
}

std::unordered_map<uint64_t, FontMetrics>& metricsCache() {
    static std::unordered_map<uint64_t, FontMetrics> cache;
    return cache;
}

// Selects a GDI font matching `sizePx` into a screen DC for the lifetime of the
// scope. Only used when DirectWrite is unavailable.
struct GdiFontScope {
    HDC hdc = nullptr;
    HFONT font = nullptr;
    HGDIOBJ previous = nullptr;

    GdiFontScope(const wchar_t* family, float sizePx) {
        hdc = GetDC(nullptr);
        if (!hdc) return;
        // A DirectWrite font size is in DIPs, which at our 96 DPI render target
        // is pixels. A negative CreateFontW height is also pixels, so ask for
        // the size directly instead of round-tripping through points the way
        // the old per-widget GDI code did (that inflated every fallback glyph
        // by 96/72).
        int height = -(int)std::lround(sizePx);
        if (height == 0) height = -1;
        font = CreateFontW(height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, family);
        if (font) previous = SelectObject(hdc, font);
    }

    ~GdiFontScope() {
        if (!hdc) return;
        if (previous) SelectObject(hdc, previous);
        if (font) DeleteObject(font);
        ReleaseDC(nullptr, hdc);
    }

    bool valid() const { return hdc != nullptr && font != nullptr; }
};

bool isBreakSpace(wchar_t c) {
    return c == L' ' || c == L'\t';
}

} // namespace

FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}

void FontManager::setFactory(IDWriteFactory* factory) {
    if (mFactory == factory) return;
    mFactory = factory;
    // Formats and metrics were produced by the previous factory/font.
    formatCache().clear();
    metricsCache().clear();
}

void FontManager::setFamilyName(const wchar_t* family) {
    if (!family || (mFamilyName && wcscmp(mFamilyName, family) == 0)) return;
    mFamilyName = family;
    formatCache().clear();
    metricsCache().clear();
}

IDWriteTextFormat* FontManager::getTextFormat(const Paint& paint) {
    if (!mFactory) return nullptr;

    const uint64_t key = sizeKey(paint);
    auto& cache = formatCache();
    auto it = cache.find(key);
    if (it != cache.end()) return it->second.Get();

    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = mFactory->CreateTextFormat(
        mFamilyName, nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        paint.getTextSize(), L"en-us", &format);
    if (FAILED(hr) || !format) {
        Logger::e("FontManager", "CreateTextFormat failed, hr=" + std::to_string((long)hr));
        return nullptr;
    }

    // Wrapping is decided by the layout width we pass in, so the format itself
    // always allows it.
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

    auto inserted = cache.emplace(key, format);
    return inserted.first->second.Get();
}

float FontManager::measureText(const std::wstring& text, const Paint& paint) {
    if (text.empty()) return 0.0f;

    IDWriteTextFormat* format = getTextFormat(paint);
    if (mFactory && format) {
        ComPtr<IDWriteTextLayout> layout;
        HRESULT hr = mFactory->CreateTextLayout(text.c_str(), (UINT32)text.length(), format,
                                                UNBOUNDED_WIDTH, UNBOUNDED_WIDTH, &layout);
        if (SUCCEEDED(hr) && layout) {
            DWRITE_TEXT_METRICS metrics = {};
            if (SUCCEEDED(layout->GetMetrics(&metrics))) {
                return metrics.widthIncludingTrailingWhitespace;
            }
        }
    }

    GdiFontScope gdi(mFamilyName, paint.getTextSize());
    if (gdi.valid()) {
        SIZE size = {};
        if (GetTextExtentPoint32W(gdi.hdc, text.c_str(), (int)text.length(), &size)) {
            return (float)size.cx;
        }
    }

    // Last resort: an average-advance guess. Visibly wrong, but never zero,
    // which would collapse the view entirely.
    return (float)text.length() * paint.getTextSize() * 0.5f;
}

FontMetrics FontManager::getFontMetrics(const Paint& paint) {
    const uint64_t key = sizeKey(paint);
    auto& cache = metricsCache();
    auto cached = cache.find(key);
    if (cached != cache.end()) return cached->second;

    FontMetrics fm;
    bool resolved = false;

    IDWriteTextFormat* format = getTextFormat(paint);
    if (mFactory && format) {
        // Line metrics from a probe string give exactly the vertical geometry
        // DirectWrite will use when it lays out real text at this size, which
        // is what we need these numbers to agree with.
        static const wchar_t kProbe[] = L"Ag";
        ComPtr<IDWriteTextLayout> probe;
        HRESULT hr = mFactory->CreateTextLayout(kProbe, 2, format,
                                                UNBOUNDED_WIDTH, UNBOUNDED_WIDTH, &probe);
        if (SUCCEEDED(hr) && probe) {
            UINT32 lineCount = 0;
            probe->GetLineMetrics(nullptr, 0, &lineCount);
            if (lineCount > 0) {
                std::vector<DWRITE_LINE_METRICS> lines(lineCount);
                if (SUCCEEDED(probe->GetLineMetrics(lines.data(), lineCount, &lineCount)) &&
                    lineCount > 0) {
                    fm.ascent = -lines[0].baseline;
                    fm.descent = lines[0].height - lines[0].baseline;
                    fm.top = fm.ascent;
                    fm.bottom = fm.descent;
                    fm.leading = 0.0f;
                    resolved = true;
                }
            }
        }
    }

    if (!resolved) {
        GdiFontScope gdi(mFamilyName, paint.getTextSize());
        TEXTMETRICW tm = {};
        if (gdi.valid() && GetTextMetricsW(gdi.hdc, &tm)) {
            fm.ascent = -(float)tm.tmAscent;
            fm.descent = (float)tm.tmDescent;
            fm.top = fm.ascent;
            fm.bottom = fm.descent;
            fm.leading = (float)tm.tmExternalLeading;
            resolved = true;
        }
    }

    if (!resolved) {
        // Rough Roboto-like split of the em box, so nothing downstream has to
        // cope with a zero line height.
        fm.ascent = -paint.getTextSize() * 0.8f;
        fm.descent = paint.getTextSize() * 0.2f;
        fm.top = fm.ascent;
        fm.bottom = fm.descent;
    }

    cache.emplace(key, fm);
    return fm;
}

std::shared_ptr<TextLayout> FontManager::getTextLayout(const std::wstring& text,
                                                       const Paint& paint, float maxWidth) {
    auto layout = std::make_shared<TextLayout>();
    layout->mText = text;
    layout->mTextSize = paint.getTextSize();
    layout->mMaxWidth = maxWidth;
    layout->mFontMetrics = getFontMetrics(paint);

    const float wrapWidth = (maxWidth > 0.0f) ? maxWidth : UNBOUNDED_WIDTH;

    if (text.empty()) {
        // AOSP reserves a full line for empty text, which is why an empty
        // wrap_content TextView on a device is a line tall rather than
        // collapsed to nothing.
        TextLayout::Line line;
        line.height = layout->mFontMetrics.getLineHeight();
        line.baseline = -layout->mFontMetrics.ascent;
        layout->mLines.push_back(line);
        layout->mHeight = line.height;
        return layout;
    }

    IDWriteTextFormat* format = getTextFormat(paint);
    if (mFactory && format) {
        ComPtr<IDWriteTextLayout> dwLayout;
        HRESULT hr = mFactory->CreateTextLayout(text.c_str(), (UINT32)text.length(), format,
                                                wrapWidth, UNBOUNDED_WIDTH, &dwLayout);
        if (SUCCEEDED(hr) && dwLayout) {
            UINT32 lineCount = 0;
            dwLayout->GetLineMetrics(nullptr, 0, &lineCount);
            if (lineCount > 0) {
                std::vector<DWRITE_LINE_METRICS> lines(lineCount);
                if (SUCCEEDED(dwLayout->GetLineMetrics(lines.data(), lineCount, &lineCount))) {
                    uint32_t offset = 0;
                    float y = 0.0f;
                    float widest = 0.0f;

                    for (UINT32 i = 0; i < lineCount; ++i) {
                        const DWRITE_LINE_METRICS& lm = lines[i];

                        // DWRITE_LINE_METRICS::length counts the line break too,
                        // and trailingWhitespaceLength counts the break plus any
                        // spaces before it. Strip both: a stray CR/LF handed to
                        // drawText would render as a box, and a trailing space
                        // would bias centred text half a space to the left.
                        uint32_t visible = lm.length;
                        if (lm.trailingWhitespaceLength <= visible) {
                            visible -= lm.trailingWhitespaceLength;
                        }

                        TextLayout::Line line;
                        line.start = offset;
                        line.length = visible;
                        line.top = y;
                        line.height = lm.height;
                        line.baseline = y + lm.baseline;
                        // Measured as its own single-line run, because that is
                        // exactly what Canvas::drawText will do with it. Reusing
                        // the wrapped layout's own per-line width would be a
                        // different number, and centring would be off by the
                        // difference.
                        line.width = measureText(text.substr(line.start, line.length), paint);

                        widest = (std::max)(widest, line.width);
                        y += line.height;
                        offset += lm.length;
                        layout->mLines.push_back(line);
                    }

                    layout->mWidth = widest;
                    layout->mHeight = y;
                    return layout;
                }
            }
        }
        Logger::e("FontManager", "DirectWrite layout failed; falling back to GDI wrapping");
    }

    wrapWithFallback(*layout, paint, wrapWidth);
    return layout;
}

void FontManager::wrapWithFallback(TextLayout& layout, const Paint& paint, float maxWidth) {
    const std::wstring& text = layout.mText;
    const float lineHeight = layout.mFontMetrics.getLineHeight();
    const float baselineOffset = -layout.mFontMetrics.ascent;

    float y = 0.0f;
    float widest = 0.0f;
    size_t cursor = 0;
    const size_t end = text.size();

    while (cursor <= end) {
        // Explicit line breaks always win over width-based wrapping.
        size_t hardBreak = text.find_first_of(L"\r\n", cursor);
        const size_t segmentEnd = (hardBreak == std::wstring::npos) ? end : hardBreak;

        size_t lineStart = cursor;
        do {
            // Greedily extend to the last space-delimited boundary that fits.
            size_t accepted = 0;
            size_t probe = lineStart;
            while (probe < segmentEnd) {
                size_t nextSpace = text.find(L' ', probe);
                size_t candidateEnd =
                    (nextSpace == std::wstring::npos || nextSpace >= segmentEnd)
                        ? segmentEnd
                        : nextSpace + 1;

                float width = measureText(text.substr(lineStart, candidateEnd - lineStart), paint);
                if (width > maxWidth && accepted > 0) break;
                accepted = candidateEnd - lineStart;
                if (width > maxWidth) break;  // single word wider than the line
                probe = candidateEnd;
            }
            if (accepted == 0) accepted = (segmentEnd > lineStart) ? 1 : 0;

            size_t lineEnd = lineStart + accepted;
            size_t visibleEnd = lineEnd;
            while (visibleEnd > lineStart && isBreakSpace(text[visibleEnd - 1])) visibleEnd--;

            TextLayout::Line line;
            line.start = (uint32_t)lineStart;
            line.length = (uint32_t)(visibleEnd - lineStart);
            line.top = y;
            line.height = lineHeight;
            line.baseline = y + baselineOffset;
            line.width = measureText(text.substr(line.start, line.length), paint);

            widest = (std::max)(widest, line.width);
            y += lineHeight;
            layout.mLines.push_back(line);

            lineStart = lineEnd;
        } while (lineStart < segmentEnd);

        if (segmentEnd >= end) break;

        // Consume the break itself: CRLF counts as one.
        cursor = segmentEnd + 1;
        if (text[segmentEnd] == L'\r' && cursor < end && text[cursor] == L'\n') cursor++;
    }

    if (layout.mLines.empty()) {
        TextLayout::Line line;
        line.height = lineHeight;
        line.baseline = baselineOffset;
        layout.mLines.push_back(line);
        y = lineHeight;
    }

    layout.mWidth = widest;
    layout.mHeight = y;
}

} // namespace graphics
} // namespace setu
