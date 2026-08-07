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

std::pair<std::vector<uint8_t>, const DexParser*> MultiDexManager::getMethodBytecode(const std::string& className, const std::string& methodName) const {
    for (const auto& dex : m_dexFiles) {
        std::vector<uint8_t> bytecode = dex->getMethodBytecode(className, methodName);
        if (!bytecode.empty()) {
            return {bytecode, dex.get()};
        }
    }
    return {{}, nullptr};
}
