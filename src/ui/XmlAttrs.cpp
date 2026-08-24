#include "XmlAttrs.h"

#include "AndroidAttrs.h"
#include "ColorStateListInflater.h"
#include "Theme.h"
#include "WindowManager.h"
#include "androidfw/Util.h"
#include "../dex/ResourceManager.h"

namespace setu {

namespace {

const char* const ANDROID_NS = "http://schemas.android.com/apk/res/android";

std::string utf16(const char16_t* str, size_t len) {
    if (!str || len == 0) return std::string();
    return android::util::Utf16ToUtf8(android::StringPiece16(str, len));
}

// An app can declare its own attribute with a framework name (app:radius next to
// android:radius), so a bare name match is not quite enough. Compiled drawables
// under res/drawable are android:-only in practice, and some binary XML carries
// no namespace string at all, so absence is treated as a match rather than a
// rejection.
bool isAndroidNamespace(const android::ResXMLParser* parser, size_t index) {
    size_t len = 0;
    const char16_t* ns16 = parser->getAttributeNamespace(index, &len);
    if (!ns16 || len == 0) return true;
    return utf16(ns16, len) == ANDROID_NS;
}

} // namespace

std::string elementName(const android::ResXMLParser* parser) {
    if (!parser) return std::string();
    size_t len = 0;
    const char16_t* name16 = parser->getElementName(&len);
    if (!name16 || len == 0) return std::string();
    return utf16(name16, len);
}

void skipCurrentElement(android::ResXMLParser* parser) {
    if (!parser) return;
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            ++depth;
        } else if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) return;
            --depth;
        }
    }
}

bool readColor(const XmlAttrs& attrs, const char* name, uint32_t& out) {
    android::AssetManager2::SelectedValue val;
    if (!attrs.getValue(name, val)) return false;

    // TypedArray.getColor accepts the whole integer range, not just the colour
    // types: #ff0000 and 0xffff0000 both land here. A TYPE_STRING instead is a
    // res/color/*.xml ColorStateList, which getColorStateList handles -
    // reporting "no colour" is what keeps that case distinguishable from a fill
    // the author really did author as fully transparent.
    if (val.type < android::Res_value::TYPE_FIRST_INT ||
        val.type > android::Res_value::TYPE_LAST_INT) {
        return false;
    }
    out = val.data;
    return true;
}

float complexToDimensionPx(uint32_t data) {
    // Arithmetic lives in the header so ViewGroup can reuse it without dragging
    // WindowManager into the standalone view-layer build.
    return complexToDimensionPxWith(data, WindowManager::getDensity(),
                                    WindowManager::getScaledDensity());
}

float complexToFraction(uint32_t data) {
    float value = (float)(int32_t(data & 0xFFFFFF00));
    const int radix =
        (data >> android::Res_value::COMPLEX_RADIX_SHIFT) & android::Res_value::COMPLEX_RADIX_MASK;

    const float MANTISSA_MULT = 1.0f / (1 << 8);
    static const float RADIX_MULTS[] = {
        1.0f * MANTISSA_MULT,
        1.0f / (1 << 7) * MANTISSA_MULT,
        1.0f / (1 << 15) * MANTISSA_MULT,
        1.0f / (1 << 23) * MANTISSA_MULT
    };
    // Both fraction units (of-self and of-parent) are already stored as a
    // fraction of 1, so there is no unit multiplier to apply.
    return value * RADIX_MULTS[radix];
}

XmlAttrs::XmlAttrs(const android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme)
    : mParser(parser), mResManager(resManager), mTheme(theme) {
}

int XmlAttrs::indexOf(const char* name) const {
    if (!mParser || !name) return -1;

    const size_t count = mParser->getAttributeCount();
    uint32_t wantedId = 0;
    bool wantedIdLooked = false;

    for (size_t i = 0; i < count; ++i) {
        if (!isAndroidNamespace(mParser, i)) continue;

        size_t len = 0;
        const char16_t* name16 = mParser->getAttributeName(i, &len);
        if (name16 && len > 0) {
            if (utf16(name16, len) == name) return (int)i;
            continue;
        }

        // Name strings stripped from the pool: compare compiled IDs instead.
        if (!wantedIdLooked) {
            wantedId = androidAttr(mResManager, name);
            wantedIdLooked = true;
        }
        if (wantedId != 0 && mParser->getAttributeNameResID(i) == wantedId) return (int)i;
    }
    return -1;
}

bool XmlAttrs::resolve(int index, android::AssetManager2::SelectedValue& out) const {
    if (index < 0 || !mParser) return false;

    android::Res_value raw;
    if (mParser->getAttributeValue((size_t)index, &raw) < 0) return false;

    out = android::AssetManager2::SelectedValue();
    out.type = raw.dataType;
    out.data = raw.data;
    out.cookie = android::kInvalidCookie;
    out.flags = 0;
    out.resid = 0;

    if (out.type == android::Res_value::TYPE_NULL) return false;

    if (out.type == android::Res_value::TYPE_REFERENCE ||
        out.type == android::Res_value::TYPE_ATTRIBUTE) {
        if (!mResManager || !mResManager->resolveValue(out, mTheme)) return false;
    }
    return true;
}

bool XmlAttrs::getValue(const char* name, android::AssetManager2::SelectedValue& out) const {
    return resolve(indexOf(name), out);
}

uint32_t XmlAttrs::getColor(const char* name, uint32_t def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    // TypedArray.getColor accepts the whole integer range, not just the colour
    // types: #ff0000 and 0xffff0000 both end up here.
    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return val.data;
    }
    // A TYPE_STRING here is a res/color/*.xml ColorStateList, which cannot be
    // flattened to one colour without knowing the view's state. Callers that can
    // hold a list should ask getColorStateList instead; for the rest, the
    // caller's default stands rather than a wrong flat colour.
    return def;
}

graphics::ColorStateListPtr XmlAttrs::getColorStateList(const char* name) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return nullptr;

    // An inline colour is a perfectly good constant list, and it is also how a
    // ?attr/ reference arrives once the theme has resolved it.
    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return graphics::ColorStateList::valueOf(val.data);
    }
    if (val.resid != 0) {
        return ColorStateListInflater::inflate(mResManager, mTheme, val.resid);
    }
    return nullptr;
}

float XmlAttrs::getDimension(const char* name, float def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    if (val.type == android::Res_value::TYPE_DIMENSION) {
        return complexToDimensionPx(val.data);
    }
    if (val.type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = val.data;
        return u.f;
    }
    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return (float)(int32_t)val.data;
    }
    return def;
}

int XmlAttrs::getDimensionPixelSize(const char* name, int def) const {
    if (!has(name)) return def;
    return dimensionPixelSize(getDimension(name, (float)def));
}

int XmlAttrs::getDimensionPixelOffset(const char* name, int def) const {
    if (!has(name)) return def;
    return dimensionPixelOffset(getDimension(name, (float)def));
}

int XmlAttrs::getInt(const char* name, int def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return (int32_t)val.data;
    }
    if (val.type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = val.data;
        return (int)u.f;
    }
    return def;
}

float XmlAttrs::getFloat(const char* name, float def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    if (val.type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = val.data;
        return u.f;
    }
    if (val.type == android::Res_value::TYPE_DIMENSION) {
        return complexToDimensionPx(val.data);
    }
    if (val.type == android::Res_value::TYPE_FRACTION) {
        return complexToFraction(val.data);
    }
    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return (float)(int32_t)val.data;
    }
    return def;
}

float XmlAttrs::getFraction(const char* name, float def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    if (val.type == android::Res_value::TYPE_FRACTION) {
        return complexToFraction(val.data);
    }
    if (val.type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = val.data;
        return u.f;
    }
    return def;
}

bool XmlAttrs::getBool(const char* name, bool def) const {
    android::AssetManager2::SelectedValue val;
    if (!resolve(indexOf(name), val)) return def;

    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return val.data != 0;
    }
    return def;
}

std::string XmlAttrs::getString(const char* name) const {
    const int index = indexOf(name);
    if (index < 0 || !mParser) return std::string();

    // A literal string lives in the document's own pool, so it needs no
    // resource lookup at all.
    if (mParser->getAttributeDataType((size_t)index) == android::Res_value::TYPE_STRING) {
        size_t len = 0;
        return utf16(mParser->getAttributeStringValue((size_t)index, &len), len);
    }

    android::AssetManager2::SelectedValue val;
    if (!resolve(index, val)) return std::string();
    if (val.type == android::Res_value::TYPE_STRING && mResManager) {
        return mResManager->getString(val.resid);
    }
    return std::string();
}

uint32_t XmlAttrs::getResourceId(const char* name) const {
    android::AssetManager2::SelectedValue val;
    const int index = indexOf(name);
    if (index < 0 || !mParser) return 0;

    // Before resolution the reference's own target is right there in the data
    // word; use it when resolution is not possible (no framework loaded).
    const int rawType = mParser->getAttributeDataType((size_t)index);
    const uint32_t rawData = (uint32_t)mParser->getAttributeData((size_t)index);

    if (resolve(index, val) && val.resid != 0) return val.resid;
    if (rawType == android::Res_value::TYPE_REFERENCE) return rawData;
    return 0;
}

} // namespace setu
