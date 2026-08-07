#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "Value.h"
#include "InterpreterState.h"

// StubFunc signature: takes InterpreterState, args array, and an output Value pointer for returns.
// Returns a boolean indicating if a Java Exception was "thrown" (true = exception).
using StubFunc = std::function<bool(InterpreterState* state, const std::vector<Value>& args, Value* outReturn)>;

class StubRegistry {
public:
    static void init();
    static bool invoke(const std::string& methodSignature, InterpreterState* state, const std::vector<Value>& args, Value* outReturn);

private:
    static std::unordered_map<std::string, StubFunc> stubs;
    
    // Register individual stubs
    static void registerActivityStubs();
    static void registerViewStubs();
};
