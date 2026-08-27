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
#include <vector>
#include <string>
#include "androidfw/ResourceTypes.h"
#include "androidfw/AssetManager2.h"
#include "Theme.h"
#include "../graphics/drawable/Drawable.h"
#include "../graphics/ColorStateList.h"

namespace setu {

class TypedArray {
public:
    TypedArray(ResourceManager* resManager, const std::vector<uint32_t>& styleables);
    ~TypedArray();

    // Populates this TypedArray with values.
    void obtainStyledAttributes(const Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);

    bool hasValue(int index) const;
    bool getBoolean(int index, bool defValue) const;
    int getInt(int index, int defValue) const;
    float getFloat(int index, float defValue) const;
    uint32_t getColor(int index, uint32_t defValue) const;
    int getDimensionPixelSize(int index, int defValue) const;
    int getLayoutDimension(int index, int defValue) const;
    std::string getString(int index) const;

    // The attribute as a Drawable, whatever form it arrived in: a colour becomes
    // a ColorDrawable, a resource is inflated through DrawableInflater. This is
    // how a widget picks up the background its *style* declares - most real
    // Material widgets never mention android:background in the layout at all.
    // nullptr when the attribute is absent or is a drawable kind not yet
    // supported.
    graphics::DrawablePtr getDrawable(int index) const;

    // The attribute as a ColorStateList, whatever form it arrived in: an inline
    // colour becomes a constant one-item list, a res/color/*.xml reference is
    // inflated. This is what android:textColor needs - a stock Material text
    // colour is a selector with a disabled entry, and getColor() reports nothing
    // at all for one of those because it is a reference, not a colour int.
    // nullptr when the attribute is absent or is not a colour resource.
    graphics::ColorStateListPtr getColorStateList(int index) const;

private:
    ResourceManager* m_resManager;
    // Kept from obtainStyledAttributes so getDrawable() can resolve nested
    // ?attr/ references inside the drawable it opens.
    Theme* m_theme = nullptr;
    std::vector<uint32_t> m_styleables;
    std::vector<android::Res_value> m_values;
    std::vector<std::string> m_stringValues;
    // The resource ID each value resolved from, which is what getDrawable needs
    // in order to open the drawable's own XML.
    std::vector<uint32_t> m_resIds;
    std::vector<bool> m_hasValue;
};

} // namespace setu

