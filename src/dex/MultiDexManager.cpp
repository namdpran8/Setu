#include "MultiDexManager.h"
#include "../utils/Logger.h"

MultiDexManager::MultiDexManager() {
}

MultiDexManager::~MultiDexManager() {
}

bool MultiDexManager::addDex(std::vector<uint8_t> dexBuffer) {
    auto bufferPtr = std::make_unique<std::vector<uint8_t>>(std::move(dexBuffer));
    auto parser = std::make_unique<DexParser>();
    
    if (parser->parse(*bufferPtr)) {
        m_dexFiles.push_back(std::move(parser));
        m_buffers.push_back(std::move(bufferPtr));
        return true;
    }
    return false;
}

Value MultiDexManager::getStaticFieldValue(const std::string& className, const std::string& fieldName) const {
    // First check the mutation cache
    std::string key = className + "->" + fieldName;
    auto it = m_staticFields.find(key);
    if (it != m_staticFields.end()) {
        return it->second;
    }
    
    // Fall back to parsing from DEX
    for (const auto& dex : m_dexFiles) {
        Value val = dex->getStaticFieldValueByName(className, fieldName);
        if (val.type != ValueType::UNINITIALIZED) {
            return val;
        }
    }
    
    // If not found in any loaded DEX
    Logger::w("MultiDexManager", "Field " + className + "->" + fieldName + " not found in ANY loaded DEX file!");
    return Value::MakeNull();
}

void MultiDexManager::setStaticFieldValue(const std::string& className, const std::string& fieldName, const Value& val) {
    std::string key = className + "->" + fieldName;
    m_staticFields[key] = val;
    Logger::d("MultiDexManager", "setStaticFieldValue: " + key);
}

std::pair<DexParser::MethodBytecodeResult, const DexParser*> MultiDexManager::getMethodBytecode(const std::string& methodSignature) const {
    for (const auto& dex : m_dexFiles) {
        auto result = dex->getMethodBytecode(methodSignature);
        if (!result.bytecode.empty()) {
            return {result, dex.get()};
        }
    }
    return {{}, nullptr};
}
