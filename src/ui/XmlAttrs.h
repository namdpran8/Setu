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
#include <string>

#include "androidfw/AssetManager2.h"
#include "androidfw/ResourceTypes.h"

#include "../graphics/ColorStateList.h"

namespace setu {

class ResourceManager;
class Theme;

// Reads the attributes of the element a ResXMLParser is currently sitting on,
// following @references and ?theme attributes on the way out.
//
// LayoutInflater does this by hand: a name comparison plus a hex ID for every
// attribute, and a copy-pasted "TYPE_DIMENSION or parse the string" branch for
// every dimension. Drawable XML is almost entirely numeric attributes, so the
// pattern is worth wrapping once. Names are matched first - that keeps working
// with no framework-res loaded - and the runtime-resolved ID from androidAttr()
// is the fallback for binary XML compiled without attribute name strings.
//
// Every getter returns `def` when the attribute is absent, unreadable, or of a
// type that cannot become the requested one. Dimensions come back in pixels,
// already density-scaled.
class XmlAttrs {
public:
    XmlAttrs(const android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme);

    bool has(const char* name) const { return indexOf(name) >= 0; }

    uint32_t getColor(const char* name, uint32_t def) const;

    // The attribute as a ColorStateList: an inline colour becomes a constant
    // one-item list, a res/color/*.xml reference is inflated. nullptr when the
    // attribute is absent or names something that is not a colour resource.
    //
    // This is the getter that makes getColor's limitation survivable - a stock
    // <solid android:color="@color/button_text"> is a reference to a selector,
    // and getColor reports the caller's default for it.
    graphics::ColorStateListPtr getColorStateList(const char* name) const;

    // Raw pixel value, unrounded. Use the two below when matching AOSP's own
    // rounding for a given attribute.
    float getDimension(const char* name, float def) const;

    // TypedArray.getDimensionPixelSize: rounds to nearest, and never lets a
    // non-zero dimension collapse to 0 - which is what keeps a 0.4px hairline
    // divider visible instead of absent.
    int getDimensionPixelSize(const char* name, int def) const;

    // TypedArray.getDimensionPixelOffset: plain truncation, no rounding.
    int getDimensionPixelOffset(const char* name, int def) const;

    int getInt(const char* name, int def) const;
    float getFloat(const char* name, float def) const;
    // Percentages, e.g. android:centerX="50%" -> 0.5f.
    float getFraction(const char* name, float def) const;
    bool getBool(const char* name, bool def) const;
    std::string getString(const char* name) const;

    // The resource ID an attribute points at (@drawable/foo -> its ID), or 0.
    // References are followed, so an alias reports the ID it finally lands on.
    uint32_t getResourceId(const char* name) const;

    // The fully-resolved value, for callers that have to branch on its *type*
    // rather than coerce it to one. android:drawable on a <selector> item is the
    // case that needs it: "#ff0000" and "@drawable/foo" are both legal and become
    // completely different drawables. Also the only honest way to tell "attribute
    // absent" from "authored as 0x00000000".
    //
    // False when the attribute is absent, explicitly @null, or names a resource
    // that cannot be resolved.
    bool getValue(const char* name, android::AssetManager2::SelectedValue& out) const;

private:
    int indexOf(const char* name) const;

    // Reads attribute `index` and resolves references and theme attributes.
    // False when the attribute holds nothing usable.
    bool resolve(int index, android::AssetManager2::SelectedValue& out) const;

    const android::ResXMLParser* mParser;
    ResourceManager* mResManager;
    Theme* mTheme;
};

// The name of the element the parser is sitting on, or "" if it has none.
std::string elementName(const android::ResXMLParser* parser);

// Walks past the element the parser is sitting on, children and all, leaving it
// on the matching END_TAG. Keeps the "consumes exactly one element" contract for
// leaf and unsupported elements alike, which is what lets a caller iterate a
// container like <selector> without losing its place.
void skipCurrentElement(android::ResXMLParser* parser);

// A colour attribute, told apart from an absent one.
//
// XmlAttrs::getColor cannot distinguish "absent or unreadable" from "authored as
// 0x00000000", and for several attributes that difference decides whether the
// thing exists at all: a <solid> with no android:color has no fill, whereas one
// authored fully transparent has a fill that happens to paint nothing. Both the
// drawable inflater and the colour-selector inflater need the distinction, which
// is why it lives here rather than in either of them.
//
// False when the attribute is absent, unreadable, or is a res/color/*.xml
// reference rather than a colour int - use getColorStateList for that case.
bool readColor(const XmlAttrs& attrs, const char* name, uint32_t& out);

// Pixel value of a TYPE_DIMENSION complex data word, density-scaled.
float complexToDimensionPx(uint32_t data);

// Density-scales a raw dimension value with a given unit.
float applyDimensionPx(int unit, float value);

// The same conversion with the densities supplied by the caller. Header-inline
// and free of any WindowManager reference, so the view layer can scale a
// layout_margin without linking XmlAttrs.cpp - constraint_layout_test builds
// View/ViewGroup standalone. The one-argument form above is a thin wrapper that
// fills these in from the current display metrics; the arithmetic lives here
// once so the two can never drift.
inline float applyDimensionWith(int unit, float value, float density, float scaledDensity) {
    switch (unit) {
        case android::Res_value::COMPLEX_UNIT_PX:
            return value;
        case android::Res_value::COMPLEX_UNIT_DIP:
            return value * density;
        case android::Res_value::COMPLEX_UNIT_SP:
            return value * scaledDensity;
        case android::Res_value::COMPLEX_UNIT_PT:
            return value * density * (1.0f / 72.0f) * 160.0f;
        case android::Res_value::COMPLEX_UNIT_IN:
            return value * density * 160.0f;
        case android::Res_value::COMPLEX_UNIT_MM:
            return value * density * (1.0f / 25.4f) * 160.0f;
        default:
            return value;
    }
}

inline float complexToDimensionPxWith(uint32_t data, float density, float scaledDensity) {
    float value = (float)(int32_t(data & 0xFFFFFF00));
    const int radix =
        (data >> android::Res_value::COMPLEX_RADIX_SHIFT) & android::Res_value::COMPLEX_RADIX_MASK;

    // AOSP's fixed-point layouts: 23p0, 16p7, 8p15, 0p23.
    const float MANTISSA_MULT = 1.0f / (1 << 8);
    const float RADIX_MULTS[] = {
        1.0f * MANTISSA_MULT,
        1.0f / (1 << 7) * MANTISSA_MULT,
        1.0f / (1 << 15) * MANTISSA_MULT,
        1.0f / (1 << 23) * MANTISSA_MULT
    };
    value *= RADIX_MULTS[radix];

    return applyDimensionWith(data & android::Res_value::COMPLEX_UNIT_MASK, value, density, scaledDensity);
}

// 0..1 value of a TYPE_FRACTION complex data word.
float complexToFraction(uint32_t data);

// AOSP's two float-to-pixel conversions, split out because which one an
// attribute uses is part of matching a real device: <corners>, <stroke> and
// <size> round, <padding> truncates. Inline for the same reason as
// complexToDimensionPxWith above - ViewGroup has to round a layout_margin, and
// it cannot link XmlAttrs.cpp.
inline int dimensionPixelSize(float value) {
    const int result = (int)(value + 0.5f);
    if (result != 0) return result;
    if (value == 0.0f) return 0;
    // A dimension the author asked for should never round away to nothing.
    return value > 0.0f ? 1 : -1;
}

inline int dimensionPixelOffset(float value) {
    return (int)value;
}

} // namespace setu
