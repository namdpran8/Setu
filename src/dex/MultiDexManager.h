/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include "DexParser.h"
#include "../interpreter/Value.h"

class MultiDexManager {
public:
    MultiDexManager();
    ~MultiDexManager();

    // Extract, store, and parse a DEX file buffer
    bool addDex(std::vector<uint8_t> dexBuffer, bool isFramework = false);

    // Load all .dex files from a given directory
    size_t loadDexFilesFromDirectory(const std::string& directoryPath, bool isFramework = false);

    // Global lookup for a static field by name
    Value getStaticFieldValue(const std::string& className, const std::string& fieldName) const;

    // Set a static field value (for sput-* instructions)
    void setStaticFieldValue(const std::string& className, const std::string& fieldName, const Value& val);

    // Retrieves bytecode for a specific method signature across all DEX files.
    // Returns { MethodBytecodeResult, DexParser* } where DexParser* is the one that contained it.
    std::pair<DexParser::MethodBytecodeResult, const DexParser*> getMethodBytecode(const std::string& methodSignature) const;

    // Hierarchy resolution
    std::string getSuperClass(const std::string& className) const;
    bool isInstanceOf(const std::string& actualClass, const std::string& expectedClass) const;

private:
    std::vector<std::unique_ptr<std::vector<uint8_t>>> m_buffers;
    std::vector<std::unique_ptr<DexParser>> m_dexFiles;
    
    // Cache for mutated static fields (written by sput-*)
    mutable std::unordered_map<std::string, Value> m_staticFields;
};
