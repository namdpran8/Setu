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

    // Extracts and parses a binary XML layout given its resource ID
    std::unique_ptr<android::ResXMLTree> getLayout(uint32_t layoutId);

    // Gets a string value by resource ID
    std::string getString(uint32_t resId);

    // Resolves a dimension
    float resolveDimension(uint32_t resId);

    // Resolves references (@) and theme attributes (?)
    bool resolveValue(android::AssetManager2::SelectedValue& in_out_value, class Theme* theme = nullptr);

private:
    ApkExtractor* m_extractor; // Main app extractor
    std::vector<android::AssetManager2::ApkAssetsPtr> m_apkAssets;
    std::unique_ptr<android::AssetManager2> m_assetManager;
};

} // namespace setu

