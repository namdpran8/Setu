#include "ResourceManager.h"
#include "../utils/Logger.h"
#include "androidfw/ResourceUtils.h"

namespace windroid {

ResourceManager::ResourceManager(ApkExtractor* extractor) 
    : m_extractor(extractor) {
}

ResourceManager::~ResourceManager() {
}

bool ResourceManager::init(const std::string& apkPath) {
    auto apkAssets = android::ApkAssets::Load(apkPath);
    if (!apkAssets) {
        Logger::e("ResourceManager", "Failed to load ApkAssets for: " + apkPath);
        return false;
    }
    
    m_apkAssets.push_back(std::move(apkAssets));

    android::ResTable_config config;
    memset(&config, 0, sizeof(config));
    
    m_assetManager = std::make_unique<android::AssetManager2>();
    std::span<const android::AssetManager2::ApkAssetsPtr> span_assets(m_apkAssets.data(), m_apkAssets.size()); m_assetManager->SetApkAssets(span_assets);
    
    Logger::i("ResourceManager", "Successfully initialized AssetManager2 for app.");
    return true;
}

bool ResourceManager::loadFrameworkApk(const std::string& path) {
    auto apkAssets = android::ApkAssets::Load(path, android::PROPERTY_SYSTEM);
    if (!apkAssets) {
        Logger::e("ResourceManager", "Failed to load framework ApkAssets from: " + path);
        return false;
    }

    // Insert framework at the beginning so it gets lowest package IDs
    m_apkAssets.insert(m_apkAssets.begin(), std::move(apkAssets));

    std::span<const android::AssetManager2::ApkAssetsPtr> span_assets(m_apkAssets.data(), m_apkAssets.size()); m_assetManager->SetApkAssets(span_assets);
    
    Logger::i("ResourceManager", "Successfully loaded framework APK.");
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

std::unique_ptr<android::ResXMLTree> ResourceManager::getLayout(uint32_t layoutId) {
    if (!m_assetManager) return nullptr;

    // First, resolve the layout resource to get its file path
    auto res_name_exp = m_assetManager->GetResourceName(layoutId);
    if (!res_name_exp.has_value()) {
        Logger::e("ResourceManager", "Failed to resolve layout name for ID: " + std::to_string(layoutId));
        return nullptr;
    }

    // The name of the layout is the entry string, e.g. "activity_main"
    // Wait, the actual file path is stored as a TYPE_STRING value.
    auto res = m_assetManager->GetResource(layoutId);
    if (!res.has_value() || res->type != android::Res_value::TYPE_STRING) {
        Logger::e("ResourceManager", "Layout resource is not a string path.");
        return nullptr;
    }

    std::string path;
    auto pool = m_assetManager->GetStringPoolForCookie(res->cookie);
    if (pool) {
        auto str_exp = pool->stringAt(res->data);
        if (str_exp.has_value()) {
            path = android::util::Utf16ToUtf8(str_exp.value());
        } else {
            auto str8_exp = pool->string8At(res->data);
            if (str8_exp.has_value()) {
                path = std::string(str8_exp.value());
            }
        }
    }

    if (path.empty()) {
        Logger::e("ResourceManager", "Could not resolve layout path for ID: " + std::to_string(layoutId));
        return nullptr;
    }
    
    Logger::d("ResourceManager", "Resolved layout ID " + std::to_string(layoutId) + " to path: " + path);
    
    // Open the asset file (binary XML)
    auto asset = m_assetManager->OpenNonAsset(path, android::Asset::ACCESS_BUFFER);
    if (!asset) {
        Logger::e("ResourceManager", "Could not open layout asset: " + path);
        return nullptr;
    }

    // Pass the buffer to ResXMLTree
    auto tree = std::make_unique<android::ResXMLTree>();
    if (tree->setTo(asset->getBuffer(true), asset->getLength(), true) != android::NO_ERROR) {
        Logger::e("ResourceManager", "Failed to parse binary XML: " + path);
        return nullptr;
    }
    
    return tree;
}

} // namespace windroid






