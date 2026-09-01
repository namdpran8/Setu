/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "Value.h"
#include "InterpreterState.h"
namespace setu { class ResourceManager; }
class MultiDexManager;
// StubFunc signature: takes InterpreterState, args array, and an output Value pointer for returns.
// Returns a boolean indicating if a Java Exception was "thrown" (true = exception).
using StubFunc = std::function<bool(InterpreterState* state, const std::vector<Value>& args, Value* outReturn)>;
class StubRegistry {
public:
    static void init(setu::ResourceManager* resManager, MultiDexManager* multiDexManager);
    static bool invoke(const std::string& methodSignature, InterpreterState* state, const std::vector<Value>& args, Value* outReturn);
    static bool isStubbed(const std::string& methodSignature);
    
    // View lookup
    static void registerClickListener(int viewId, const Value& listenerObj);
    static InterpreterObject* getClickListener(int controlId);
    static InterpreterObject* getLongClickListener(int controlId);
private:
    static std::unordered_map<std::string, StubFunc> stubs;
    static setu::ResourceManager* m_resManager;
    static MultiDexManager* m_multiDexManager;
    static std::unordered_map<int, InterpreterObject*> clickListeners;
    static std::unordered_map<int, InterpreterObject*> longClickListeners;
    // Register individual stubs
    static void registerActivityStubs();
    static void registerViewStubs();
};
