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
#include <string>
#include <cstdint>
#include <memory>
#include <vector>
#include "androidfw/AssetManager2.h"
#include "androidfw/ApkAssets.h"
#include "androidfw/ResourceTypes.h"
#include "../apk_extractor/apkextractor.h"

namespace setu {

class ResourceManager {
public:
    ResourceManager(ApkExtractor* extractor);
    ~ResourceManager();

    bool init(const std::string& apkPath);
    bool loadFrameworkApk(const std::string& path);

    android::AssetManager2* getAssetManager() { return m_assetManager.get(); }

    // Extracts and parses a binary XML resource given its resource ID. Layouts,
    // drawables, animators - anything AAPT compiled to an XML file.
    std::unique_ptr<android::ResXMLTree> openXml(uint32_t resId);

    // Extracts and parses a binary XML layout given its resource ID
    std::unique_ptr<android::ResXMLTree> getLayout(uint32_t layoutId);

    // The file path a resource ID points at, e.g. "res/drawable/bg.xml". Empty
    // when the resource is not a file at all. Lets a caller tell a compiled XML
    // drawable apart from a .png before trying to parse it.
    std::string getResourceFilePath(uint32_t resId);

    // Gets a string value by resource ID
    std::string getString(uint32_t resId);

    // Resolves a dimension
    float resolveDimension(uint32_t resId);

    // Applies unit scaling to a raw dimension value
    static float applyDimension(int unit, float value);

    // Gets an integer value by resource ID
    int getInt(uint32_t resId);

    // Resolves references (@) and theme attributes (?)
    bool resolveValue(android::AssetManager2::SelectedValue& in_out_value, class Theme* theme = nullptr);

private:
    ApkExtractor* m_extractor; // Main app extractor
    std::vector<android::AssetManager2::ApkAssetsPtr> m_apkAssets;
    std::unique_ptr<android::AssetManager2> m_assetManager;
};

} // namespace setu

