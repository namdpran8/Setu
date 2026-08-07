#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "InterpreterState.h"
#include "../dex/DexParser.h"
#include "../dex/MultiDexManager.h" // Forward declaration

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    // Executes a raw array of Dalvik bytecode instructions
    // dexParser is used to resolve strings and method names for logging/dispatch
    // multiDexManager is used to resolve cross-DEX dependencies
    void executeMethod(const std::vector<uint8_t>& bytecode, const DexParser* currentDex = nullptr, const MultiDexManager* multiDexManager = nullptr);

private:
    // Helper to fetch the next byte and increment the PC
    uint8_t fetchOpcode(const std::vector<uint8_t>& bytecode, uint32_t& pc);
};
