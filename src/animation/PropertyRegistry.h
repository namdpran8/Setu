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

#include <string>
#include <unordered_map>
#include <functional>
#include "../utils/Logger.h"

namespace setu {
namespace animation {

struct FloatProperty {
    std::function<void(void* target, float value)> setter;
    std::function<float(void* target)> getter;
};

class PropertyRegistry {
public:
    static void init();
public:
    static void registerFloatProperty(const std::string& className, const std::string& propertyName, 
                                      std::function<void(void*, float)> setter, 
                                      std::function<float(void*)> getter) {
        auto& classProps = getRegistry()[className];
        classProps[propertyName] = {setter, getter};
    }

    static const FloatProperty* getFloatProperty(const std::string& className, const std::string& propertyName) {
        auto& reg = getRegistry();
        auto classIt = reg.find(className);
        if (classIt != reg.end()) {
            auto propIt = classIt->second.find(propertyName);
            if (propIt != classIt->second.end()) {
                return &propIt->second;
            }
        }
        Logger::w("PropertyRegistry", "Property not found: " + className + "." + propertyName);
        return nullptr;
    }

private:
    static std::unordered_map<std::string, std::unordered_map<std::string, FloatProperty>>& getRegistry() {
        static std::unordered_map<std::string, std::unordered_map<std::string, FloatProperty>> s_registry;
        return s_registry;
    }
};

} // namespace animation
} // namespace setu
