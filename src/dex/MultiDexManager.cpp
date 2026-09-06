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

#include "MultiDexManager.h"
#include "../utils/Logger.h"

MultiDexManager::MultiDexManager() {
}

MultiDexManager::~MultiDexManager() {
}

#include <filesystem>
#include <fstream>

bool MultiDexManager::addDex(std::vector<uint8_t> dexBuffer, bool isFramework) {
    auto bufferPtr = std::make_unique<std::vector<uint8_t>>(std::move(dexBuffer));
    auto parser = std::make_unique<DexParser>();
    
    if (parser->parse(*bufferPtr)) {
        parser->setFramework(isFramework);
        m_dexFiles.push_back(std::move(parser));
        m_buffers.push_back(std::move(bufferPtr));
        return true;
    }
    return false;
}

size_t MultiDexManager::loadDexFilesFromDirectory(const std::string& directoryPath, bool isFramework) {
    size_t classesLoaded = 0;
    std::error_code ec;
    if (!std::filesystem::exists(directoryPath, ec) || !std::filesystem::is_directory(directoryPath, ec)) {
        Logger::w("MultiDexManager", "Directory not found or not a directory: " + directoryPath);
        return 0;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directoryPath, ec)) {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".dex") {
            std::ifstream file(entry.path(), std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                Logger::e("MultiDexManager", "Failed to open dex file: " + entry.path().string());
                continue;
            }
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> buffer(size);
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                if (addDex(std::move(buffer), isFramework)) {
                    classesLoaded += m_dexFiles.back()->getClassCount();
                    Logger::i("MultiDexManager", "Loaded framework DEX: " + entry.path().filename().string());
                } else {
                    Logger::e("MultiDexManager", "Failed to parse DEX: " + entry.path().filename().string());
                }
            }
        }
    }
    return classesLoaded;
}

Value MultiDexManager::getStaticFieldValue(const std::string& className, const std::string& fieldName) const {
    if (className == "Landroid/os/Build$VERSION;" && fieldName == "SDK_INT") {
        return Value::MakeInt(33); // Mock SDK_INT
    }
    
    if (className == "Landroid/graphics/Typeface;") {
        static std::unordered_map<std::string, Value> s_typefaces;
        if (s_typefaces.empty()) {
            auto makeTypeface = []() {
                InterpreterObject* obj = new InterpreterObject();
                obj->className = "Landroid/graphics/Typeface;";
                return Value::MakeObject(obj);
            };
            s_typefaces["DEFAULT"] = makeTypeface();
            s_typefaces["DEFAULT_BOLD"] = makeTypeface();
            s_typefaces["SANS_SERIF"] = makeTypeface();
            s_typefaces["SERIF"] = makeTypeface();
            s_typefaces["MONOSPACE"] = makeTypeface();
        }
        
        auto it = s_typefaces.find(fieldName);
        if (it != s_typefaces.end()) {
            return it->second;
        }
    }
    
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

std::string MultiDexManager::getSuperClass(const std::string& className) const {
    for (const auto& dex : m_dexFiles) {
        std::string superClass = dex->getSuperClass(className);
        if (!superClass.empty()) {
            return superClass;
        }
    }
    return "";
}

MultiDexManager::ClassLocation MultiDexManager::findClass(const std::string& className) const {
    for (const auto& dex : m_dexFiles) {
        const class_def_item* classDef = dex->findClass(className);
        if (classDef) {
            return {classDef, dex.get()};
        }
    }
    // Suppress warnings for standard Android and Java classes
    if (className.find("Ljava/") != 0 && className.find("Landroid/") != 0 && className.find("Ldalvik/") != 0) {
        Logger::w("MultiDexManager", "Class " + className + " not found in ANY loaded DEX file.");
    }
    return {nullptr, nullptr};
}

bool MultiDexManager::isInstanceOf(const std::string& actualClass, const std::string& expectedClass) const {
    std::string currentClass = actualClass;
    while (!currentClass.empty()) {
        if (currentClass == expectedClass) return true;
        currentClass = getSuperClass(currentClass);
    }
    return false;
}
