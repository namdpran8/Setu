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

bool MultiDexManager::isInstanceOf(const std::string& actualClass, const std::string& expectedClass) const {
    if (actualClass == expectedClass) return true;
    if (expectedClass == "Ljava/lang/Object;") return true;

    // Hardcoded overrides for stubbed framework (similar to what was in check-cast)
    if (actualClass.find("Activity") != std::string::npos && expectedClass.find("Context") != std::string::npos) return true;
    if (expectedClass == "Ljava/lang/CharSequence;" && actualClass == "java.lang.String") return true;

    // Walk the merged hierarchy
    std::string currentClass = actualClass;
    while (!currentClass.empty()) {
        if (currentClass == expectedClass) return true;
        
        // Custom framework short-circuits (fallback if missing from DEX)
        if (currentClass.find("Activity") != std::string::npos && expectedClass.find("Context") != std::string::npos) return true;
        if (currentClass.find("TextView") != std::string::npos && expectedClass.find("View") != std::string::npos) return true;
        if (currentClass.find("EditText") != std::string::npos && expectedClass.find("TextView") != std::string::npos) return true;
        if (currentClass.find("Button") != std::string::npos && expectedClass.find("View") != std::string::npos) return true;

        currentClass = getSuperClass(currentClass);
    }
    
    // Reverse check for UI stubs: if actualClass is a stubbed generic view, and expectedClass is a custom view in DEX that inherits from it
    if (actualClass == "Landroid/view/ViewGroup;" || actualClass == "Landroid/view/View;" || actualClass == "Landroid/widget/FrameLayout;") {
        currentClass = expectedClass;
        while (!currentClass.empty()) {
            if (currentClass == actualClass || currentClass == "Landroid/view/View;") return true;
            currentClass = getSuperClass(currentClass);
        }
    }
    
    // Also, if the expected class is a generic View and actual is a ViewGroup variant not explicitly checked
    if (expectedClass.find("android/view/") != std::string::npos || 
        expectedClass.find("android/widget/") != std::string::npos ||
        expectedClass.find("androidx/") != std::string::npos ||
        expectedClass.find("com/google/android/material/") != std::string::npos) {
        // Fallback for our UI stubbing, if the above walk didn't catch it
        return true; 
    }
    
    return false;
}
