#include "ResourceManager.h"
#include "../utils/Logger.h"
#include "androidfw/ResourceUtils.h"
#include "../ui/WindowManager.h"

#include "../ui/Theme.h"

namespace setu {

ResourceManager::ResourceManager(ApkExtractor* extractor) 
    : m_extractor(extractor) {
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

bool ResourceManager::init(const std::string& apkPath) {
    auto apkAssets = android::ApkAssets::Load(apkPath);
    if (!apkAssets) {
        Logger::e("ResourceManager", "Failed to load ApkAssets for: " + apkPath);
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
    // Extract mantissa and radix using AOSP mask logic
    float value = (float)(int32_t(data & 0xFFFFFF00));
    int radix = (data >> android::Res_value::COMPLEX_RADIX_SHIFT) & android::Res_value::COMPLEX_RADIX_MASK;

    // Apply radix scaling (AOSP uses fixed-point: 23p0, 16p7, 8p15, 0p23)
    const float MANTISSA_MULT = 1.0f / (1 << 8);
    static const float RADIX_MULTS[] = {
        1.0f * MANTISSA_MULT,                    // 23p0
        1.0f / (1 << 7) * MANTISSA_MULT,         // 16p7
        1.0f / (1 << 15) * MANTISSA_MULT,        // 8p15
        1.0f / (1 << 23) * MANTISSA_MULT         // 0p23
    };
    value *= RADIX_MULTS[radix];

    // Apply unit conversion
    int unit = data & android::Res_value::COMPLEX_UNIT_MASK;
    float density = WindowManager::getDensity();
    switch (unit) {
        case android::Res_value::COMPLEX_UNIT_PX:
            return value;
        case android::Res_value::COMPLEX_UNIT_DIP:
            return value * density;
        case android::Res_value::COMPLEX_UNIT_SP:
            return value * WindowManager::getScaledDensity();
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

} // namespace setu






