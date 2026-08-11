#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include "../apk_extractor/apkextractor.h"
#include "ArscParser.h"
#include "../AxmlPraserer/AxmlParser.h"

class ResourceManager {
public:
    ResourceManager(ApkExtractor* extractor);
    ~ResourceManager();

    bool init();

    // Gets the path to a resource (e.g. 0x7f0b001c -> "res/layout/activity_main.xml")
    std::string getResourcePath(uint32_t resId) const;
    
    // Extracts and parses an AXML file given its resource ID
    std::unique_ptr<AxmlParser> getLayout(uint32_t layoutId) const;

    // Gets a string value by resource ID (same as getResourcePath, but named appropriately)
    std::string getString(uint32_t resId) const;

    // Get a complex bag (style/theme) by resource ID
    const ArscParser::Bag* getBag(uint32_t resId) const;

private:
    ApkExtractor* m_extractor;
    ArscParser m_arscParser;
};
