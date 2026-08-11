#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include "../apk_extractor/apkextractor.h"
#include "ArscParser.h"
#include "../AxmlPraserer/AxmlParser.h"
#include <unordered_map>

class ResourceManager {
public:
    ResourceManager(ApkExtractor* extractor);
    ~ResourceManager();

    bool init();
    bool loadFrameworkApk(const std::string& path);

    // Gets the path to a resource (e.g. 0x7f0b001c -> "res/layout/activity_main.xml")
    std::string getResourcePath(uint32_t resId) const;
    
    // Extracts and parses an AXML file given its resource ID
    std::unique_ptr<AxmlParser> getLayout(uint32_t layoutId) const;

    // Gets a string value by resource ID (same as getResourcePath, but named appropriately)
    std::string getString(uint32_t resId) const;

    // Get a complex bag (style/theme) by resource ID
    const ArscParser::Bag* getBag(uint32_t resId) const;

private:
    ApkExtractor* m_extractor; // Main app extractor
    std::unique_ptr<ApkExtractor> m_frameworkExtractor; // Framework APK extractor

    std::unordered_map<uint8_t, std::shared_ptr<ArscParser>> m_parsers;
};
