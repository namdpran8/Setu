#pragma once
#include <vector>
#include <cstdint>
#include <string>

// Represents the state of our virtual machine at any given moment
struct InterpreterState {
    // Dalvik is register-based. A method can use up to 256 virtual registers.
    // In a real implementation, this would be a union (int, float, object pointer),
    // but for now, we'll just use uint32_t.
    uint32_t registers[256]; 
    
    // The Program Counter (PC): Our current byte offset into the bytecode array
    uint32_t pc;
};

class DexParser; // Forward declaration

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    // Executes a raw array of Dalvik bytecode instructions
    // dexParser is used to resolve strings and method names for logging/dispatch
    void executeMethod(const std::vector<uint8_t>& bytecode, const DexParser* dexParser = nullptr);

private:
    // Helper to fetch the next byte and increment the PC
    uint8_t fetchOpcode(const std::vector<uint8_t>& bytecode, uint32_t& pc);
};
