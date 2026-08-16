#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "InterpreterState.h"
#include "../dex/DexParser.h"
#include "../dex/MultiDexManager.h"

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    // Executes a raw array of Dalvik bytecode instructions
    // dexParser is used to resolve strings and method names for logging/dispatch
    // multiDexManager is used to resolve cross-DEX dependencies (mutable for static field writes)
    Value executeMethod(const std::vector<uint8_t>& bytecode, 
                        const DexParser* currentDex = nullptr, 
                        MultiDexManager* multiDexManager = nullptr,
                        const std::vector<Value>& args = {},
                        uint16_t registers_size = 0,
                        uint16_t ins_size = 0);

private:
    // Helper to fetch the next byte and increment the PC
    uint8_t fetchOpcode(const std::vector<uint8_t>& bytecode, uint32_t& pc);
    uint8_t safe8(const std::vector<uint8_t>& bytecode, uint32_t offset);
    uint16_t safe16(const std::vector<uint8_t>& bytecode, uint32_t offset);
};
