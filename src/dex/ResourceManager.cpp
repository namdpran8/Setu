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
        Logger::e("ResourceManager", "Could not extract resources.arsc from app");
        return false;
    }
    
    auto parser = std::make_shared<ArscParser>();
    if (!parser->parse(arscBuffer)) {
        Logger::e("ResourceManager", "Failed to parse app resources.arsc");
        return false;
    }
    
    uint8_t pkgId = parser->getPackageId();
    m_parsers[pkgId] = parser;
    
    Logger::i("ResourceManager", "Successfully initialized app resources.arsc (Package ID: " + std::to_string(pkgId) + ")");
    return true;
}

bool ResourceManager::loadFrameworkApk(const std::string& path) {
    m_frameworkExtractor = std::make_unique<ApkExtractor>();
    if (!m_frameworkExtractor->OpenApk(path)) {
        Logger::e("ResourceManager", "Failed to open framework APK at: " + path);
        return false;
    }

    std::vector<uint8_t> arscBuffer;
    if (!m_frameworkExtractor->ExtractEntryToMemory("resources.arsc", arscBuffer)) {
        Logger::e("ResourceManager", "Could not extract resources.arsc from framework APK");
        return false;
    }

    auto parser = std::make_shared<ArscParser>();
    if (!parser->parse(arscBuffer)) {
        Logger::e("ResourceManager", "Failed to parse framework resources.arsc");
        return false;
    }

    uint8_t pkgId = parser->getPackageId();
    m_parsers[pkgId] = parser;

    Logger::i("ResourceManager", "Successfully initialized framework resources.arsc (Package ID: " + std::to_string(pkgId) + ")");
    return true;
}

std::string ResourceManager::getResourcePath(uint32_t resId) const {
    uint8_t pkgId = (resId >> 24) & 0xFF;
    auto it = m_parsers.find(pkgId);
    if (it != m_parsers.end()) {
        return it->second->resolveStringValue(resId);
    }
    return "";
}

std::string ResourceManager::getString(uint32_t resId) const {
    uint8_t pkgId = (resId >> 24) & 0xFF;
    auto it = m_parsers.find(pkgId);
    if (it != m_parsers.end()) {
        return it->second->resolveStringValue(resId);
    }
    return "";
}

const ArscParser::Bag* ResourceManager::getBag(uint32_t resId) const {
    uint8_t pkgId = (resId >> 24) & 0xFF;
    auto it = m_parsers.find(pkgId);
    if (it != m_parsers.end()) {
        return it->second->getBag(resId);
    }
    return nullptr;
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
