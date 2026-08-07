#pragma once
#include <vector>
#include <memory>
#include <string>
#include "DexParser.h"
#include "../interpreter/Value.h"

class MultiDexManager {
public:
    MultiDexManager();
    ~MultiDexManager();

    // Extract, store, and parse a DEX file buffer
    bool addDex(std::vector<uint8_t> dexBuffer);

    // Global lookup for a static field by name
    Value getStaticFieldValue(const std::string& className, const std::string& fieldName) const;

    // Helper to find bytecode and the DexParser it belongs to
    std::pair<std::vector<uint8_t>, const DexParser*> getMethodBytecode(const std::string& className, const std::string& methodName) const;

private:
    std::vector<std::unique_ptr<std::vector<uint8_t>>> m_buffers;
    std::vector<std::unique_ptr<DexParser>> m_dexFiles;
};
