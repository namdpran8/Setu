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

#include "ResourceManager.h"
#include "../utils/Logger.h"
#include "androidfw/ResourceUtils.h"
#include "../ui/WindowManager.h"

#include "../ui/Theme.h"
#include "../ui/XmlAttrs.h"

#include "androidfw/AssetsProvider.h"

namespace setu {

ResourceManager::ResourceManager() {
}

ResourceManager::~ResourceManager() {
}

bool ResourceManager::resolveValue(android::AssetManager2::SelectedValue& in_out_value, Theme* theme) {
    if (!m_assetManager) return false;

    if (theme && theme->getTheme()) {
        auto res = theme->getTheme()->ResolveAttributeReference(in_out_value);
        return res.has_value();
    } else {
        auto res = m_assetManager->ResolveReference(in_out_value);
        return res.has_value();
    }
}

bool ResourceManager::init(const std::string& path, bool isDirectory) {
    android::AssetManager2::ApkAssetsPtr apkAssets;
    
    if (isDirectory) {
        auto provider = android::DirectoryAssetsProvider::Create(path);
        apkAssets = android::ApkAssets::Load(std::move(provider));
    } else {
        apkAssets = android::ApkAssets::Load(path);
    }
    
    if (!apkAssets) {
        Logger::e("ResourceManager", "Failed to load ApkAssets for: " + path);
        return false;
    }
    
    m_apkAssets.push_back(std::move(apkAssets));

    android::ResTable_config config;
    memset(&config, 0, sizeof(config));
    // Set basic display metrics for resource filtering
    config.density = (uint16_t)(WindowManager::getDensity() * 160);
    config.screenWidthDp = 411;   // Reasonable phone default
    config.screenHeightDp = 731;
    config.sdkVersion = 33;       // Target Android 13
    config.orientation = android::ResTable_config::ORIENTATION_PORT;
    
    m_assetManager = std::make_unique<android::AssetManager2>();
    m_assetManager->SetConfigurations({{config}});

    std::span<const android::AssetManager2::ApkAssetsPtr> span_assets(m_apkAssets.data(), m_apkAssets.size());
    m_assetManager->SetApkAssets(span_assets);
    
    Logger::i("ResourceManager", "Config: density=" + std::to_string(config.density) + 
                                 ", screenWidthDp=" + std::to_string(config.screenWidthDp) +
                                 ", screenHeightDp=" + std::to_string(config.screenHeightDp) +
                                 ", sdkVersion=" + std::to_string(config.sdkVersion) +
                                 ", orientation=" + std::to_string(config.orientation));
    Logger::i("ResourceManager", "Successfully initialized AssetManager2 for app.");
    return true;
}

bool ResourceManager::loadFrameworkApk(const std::string& path) {
    Logger::i("ResourceManager", "Attempting to load framework APK from: " + path);
    auto apkAssets = android::ApkAssets::Load(path, android::PROPERTY_SYSTEM);
    if (!apkAssets) {
        Logger::e("ResourceManager", "Failed to load framework ApkAssets from: " + path);
        return false;
    }

    // Insert framework at the beginning so it gets lowest package IDs
    m_apkAssets.insert(m_apkAssets.begin(), std::move(apkAssets));

    std::span<const android::AssetManager2::ApkAssetsPtr> span_assets(m_apkAssets.data(), m_apkAssets.size()); m_assetManager->SetApkAssets(span_assets);
    
    Logger::i("ResourceManager", "Successfully loaded framework APK from: " + path);
    return true;
}

std::string ResourceManager::getString(uint32_t resId) {
    if (!m_assetManager) return "";

    auto res = m_assetManager->GetResource(resId);
    if (!res.has_value()) {
        return "";
    }

    if (res->type == android::Res_value::TYPE_STRING) {
        auto pool = m_assetManager->GetStringPoolForCookie(res->cookie);
        if (pool) {
            auto str_exp = pool->stringAt(res->data);
            if (str_exp.has_value()) {
                return android::util::Utf16ToUtf8(str_exp.value());
            }
            auto str8_exp = pool->string8At(res->data);
            if (str8_exp.has_value()) {
                return std::string(str8_exp.value());
            }
        }
    }
    return "";
}

static float complexToDimension(uint32_t data) {
    // Unit and radix decoding lives in one place now; see ui/XmlAttrs.cpp.
    return complexToDimensionPx(data);
}

float ResourceManager::resolveDimension(uint32_t resId) {
    if (!m_assetManager) return 0.0f;
    auto res = m_assetManager->GetResource(resId);
    if (!res.has_value()) return 0.0f;
    
    android::AssetManager2::SelectedValue val;
    val.type = res->type;
    val.data = res->data;
    val.cookie = res->cookie;
    val.flags = res->flags;
    val.resid = resId;

    if (!resolveValue(val, nullptr)) {
        return 0.0f;
    }

    if (val.type == android::Res_value::TYPE_DIMENSION) {
        return complexToDimension(val.data);
    } else if (val.type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = val.data;
        return u.f;
    } else if (val.type >= android::Res_value::TYPE_FIRST_INT &&
               val.type <= android::Res_value::TYPE_LAST_INT) {
        return (float)val.data;
    }
    return 0.0f;
}

float ResourceManager::applyDimension(int unit, float value) {
    return applyDimensionPx(unit, value);
}

int ResourceManager::getInt(uint32_t resId) {
    if (!m_assetManager) return 0;
    auto res = m_assetManager->GetResource(resId);
    if (!res.has_value()) return 0;
    
    if (res->type >= android::Res_value::TYPE_FIRST_INT &&
        res->type <= android::Res_value::TYPE_LAST_INT) {
        return (int)res->data;
    }
    return 0;
}

std::string ResourceManager::getResourceFilePath(uint32_t resId) {
    if (!m_assetManager) return "";

    auto res = m_assetManager->GetResource(resId);
    if (!res.has_value()) {
        Logger::e("ResourceManager", "No value for resource ID: " + std::to_string(resId));
        return "";
    }

    // A drawable can be an alias for another drawable (@drawable/a -> @drawable/b),
    // so follow references before deciding whether this is a file at all.
    android::AssetManager2::SelectedValue val = res.value();
    if (val.type == android::Res_value::TYPE_REFERENCE) {
        if (!m_assetManager->ResolveReference(val).has_value()) {
            Logger::e("ResourceManager", "Could not resolve reference for resource ID: " + std::to_string(resId));
            return "";
        }
    }

    if (val.type != android::Res_value::TYPE_STRING) {
        // Not an error: a colour, a dimension or an inline value simply is not a file.
        return "";
    }

    auto pool = m_assetManager->GetStringPoolForCookie(val.cookie);
    if (!pool) return "";

    auto str_exp = pool->stringAt(val.data);
    if (str_exp.has_value()) {
        return android::util::Utf16ToUtf8(str_exp.value());
    }
    auto str8_exp = pool->string8At(val.data);
    if (str8_exp.has_value()) {
        return std::string(str8_exp.value());
    }
    return "";
}

std::unique_ptr<android::ResXMLTree> ResourceManager::openXml(uint32_t resId) {
    if (!m_assetManager) return nullptr;

    const std::string path = getResourceFilePath(resId);
    if (path.empty()) {
        Logger::e("ResourceManager", "Could not resolve file path for resource ID: " + std::to_string(resId));
        return nullptr;
    }

    Logger::d("ResourceManager", "Resolved resource ID " + std::to_string(resId) + " to path: " + path);

    // Open the asset file (binary XML)
    auto asset = m_assetManager->OpenNonAsset(path, android::Asset::ACCESS_BUFFER);
    if (!asset) {
        Logger::e("ResourceManager", "Could not open asset: " + path);
        return nullptr;
    }

    // copyData=true, so the tree owns its buffer and outlives the Asset.
    auto tree = std::make_unique<android::ResXMLTree>();
    if (tree->setTo(asset->getBuffer(true), asset->getLength(), true) != android::NO_ERROR) {
        Logger::e("ResourceManager", "Failed to parse binary XML: " + path);
        return nullptr;
    }

    return tree;
}

std::unique_ptr<android::ResXMLTree> ResourceManager::getLayout(uint32_t layoutId) {
    return openXml(layoutId);
}

} // namespace setu






