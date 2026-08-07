#include "Interpreter.h"
#include "../utils/Logger.h"
#include "../dex/DexParser.h"

Interpreter::Interpreter() {
}

Interpreter::~Interpreter() {
}

uint8_t Interpreter::fetchOpcode(const std::vector<uint8_t>& bytecode, uint32_t& pc) {
    if (pc >= bytecode.size()) {
        Logger::e("Interpreter", "PC out of bounds! We ran off the end of the bytecode!");
        return 0; 
    }
    return bytecode[pc++];
}

void Interpreter::executeMethod(const std::vector<uint8_t>& bytecode, const DexParser* dexParser) {
    InterpreterState state;
    state.pc = 0;
    
    // Initialize all virtual registers to 0
    for (int i = 0; i < 256; i++) {
        state.registers[i] = 0;
    }

    Logger::i("Interpreter", "Starting bytecode execution loop...");
    
    bool isRunning = true;
    while (isRunning && state.pc < bytecode.size()) {
        // 1. Fetch the opcode
        uint8_t opcode = fetchOpcode(bytecode, state.pc);
        
        // 2. Decode and Execute
        switch (opcode) {
            case 0x0E: { // return-void
                Logger::d("Interpreter", "[0x0E] return-void -> Exiting method");
                isRunning = false;
                break;
            }
            case 0x0C: { // move-result-object
                Logger::d("Interpreter", "[0x0C] move-result-object (STUB)");
                // Dalvik instructions are made of 16-bit code units (2 bytes each).
                // move-result-object is format 11x (1 code unit total = 2 bytes)
                // We consumed 1 byte for the opcode, skip 1 more.
                state.pc += 1; 
                break;
            }
            case 0x22: { // new-instance
                Logger::d("Interpreter", "[0x22] new-instance (STUB)");
                // format 21c (2 code units = 4 bytes) -> skip remaining 3
                state.pc += 3;
                break;
            }
            case 0x60: { // sget
                Logger::d("Interpreter", "[0x60] sget (STUB)");
                // format 21c (2 code units = 4 bytes) -> skip remaining 3
                state.pc += 3;
                break;
            }
            case 0x6E: { // invoke-virtual
                // Format: 6eAA BBBB CCCC
                uint16_t methodIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                std::string methodName = dexParser ? dexParser->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                Logger::d("Interpreter", "[0x6E] invoke-virtual -> would call: " + methodName);
                state.pc += 5;
                break;
            }
            case 0x6F: { // invoke-super
                uint16_t methodIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                std::string methodName = dexParser ? dexParser->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                Logger::d("Interpreter", "[0x6F] invoke-super -> would call: " + methodName);
                state.pc += 5;
                break;
            }
            case 0x70: { // invoke-direct
                uint16_t methodIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                std::string methodName = dexParser ? dexParser->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                Logger::d("Interpreter", "[0x70] invoke-direct -> would call: " + methodName);
                state.pc += 5;
                break;
            }
            case 0x71: { // invoke-static
                uint16_t methodIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                std::string methodName = dexParser ? dexParser->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                Logger::d("Interpreter", "[0x71] invoke-static -> would call: " + methodName);
                state.pc += 5;
                break;
            }
            default: {
                Logger::e("Interpreter", "UNIMPLEMENTED OPCODE: 0x" + std::to_string(opcode));
                isRunning = false; // Crash the VM!
                break;
            }
        }
    }
    
    Logger::i("Interpreter", "Execution finished.");
}
