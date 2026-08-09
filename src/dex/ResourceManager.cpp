#include "ResourceManager.h"
#include "../utils/Logger.h"

ResourceManager::ResourceManager(ApkExtractor* extractor) 
    : m_extractor(extractor) {
}

ResourceManager::~ResourceManager() {
}

bool ResourceManager::init() {
    std::vector<uint8_t> arscBuffer;
    if (!m_extractor->ExtractEntryToMemory("resources.arsc", arscBuffer)) {
        Logger::e("ResourceManager", "Could not extract resources.arsc");
        return false;
    }
    
    if (!m_arscParser.parse(arscBuffer)) {
        Logger::e("ResourceManager", "Failed to parse resources.arsc");
        return false;
    }
    
    Logger::i("ResourceManager", "Successfully initialized resources.arsc");
    return true;
}

std::string ResourceManager::getResourcePath(uint32_t resId) const {
    return m_arscParser.resolveStringValue(resId);
}

std::string ResourceManager::getString(uint32_t resId) const {
    return m_arscParser.resolveStringValue(resId);
}

std::unique_ptr<AxmlParser> ResourceManager::getLayout(uint32_t layoutId) const {
    std::string path = getResourcePath(layoutId);
    if (path.empty()) {
        Logger::e("ResourceManager", "Could not resolve layout path for ID: " + std::to_string(layoutId));
        return nullptr;
    }
    
    Logger::d("ResourceManager", "Resolved layout ID " + std::to_string(layoutId) + " to path: " + path);
    
    std::vector<uint8_t> axmlBuffer;
    if (!m_extractor->ExtractEntryToMemory(path.c_str(), axmlBuffer)) {
        Logger::e("ResourceManager", "Could not extract layout file: " + path);
        return nullptr;
    }
    
    auto parser = std::make_unique<AxmlParser>();
    if (!parser->parse(axmlBuffer)) {
        Logger::e("ResourceManager", "Failed to parse AXML layout: " + path);
        return nullptr;
    }
    
    return parser;
}
