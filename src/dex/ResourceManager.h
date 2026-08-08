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

private:
    ApkExtractor* m_extractor;
    ArscParser m_arscParser;
};
