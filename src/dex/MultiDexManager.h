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

    // Retrieves bytecode for a specific method signature across all DEX files.
    // Returns { MethodBytecodeResult, DexParser* } where DexParser* is the one that contained it.
    std::pair<DexParser::MethodBytecodeResult, const DexParser*> getMethodBytecode(const std::string& methodSignature) const;

private:
    std::vector<std::unique_ptr<std::vector<uint8_t>>> m_buffers;
    std::vector<std::unique_ptr<DexParser>> m_dexFiles;
};
