#include "Interpreter.h"
#include "../utils/Logger.h"
#include "../dex/MultiDexManager.h"
#include "StubRegistry.h"

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

void Interpreter::executeMethod(const std::vector<uint8_t>& bytecode, const DexParser* currentDex, const MultiDexManager* multiDexManager) {
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
                // Format 21c: AA|op BBBB
                uint8_t aa = bytecode[state.pc];
                uint16_t fieldIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                
                if (currentDex && multiDexManager) {
                    // Try local first
                    Value staticVal = currentDex->getStaticFieldValue(fieldIdx);
                    
                    if (staticVal.type == ValueType::UNINITIALIZED) {
                        // Not found locally? Get the string names and try global!
                        std::string className = currentDex->getClassNameFromFieldIdx(fieldIdx);
                        std::string fieldName = currentDex->getFieldNameFromFieldIdx(fieldIdx);
                        
                        if (!className.empty() && !fieldName.empty()) {
                            staticVal = multiDexManager->getStaticFieldValue(className, fieldName);
                        }
                    }
                    
                    // The standard DEX registers are just uint32_t, so we'll store ints
                    // (since resource IDs are ints). If we ever need Objects, we'll need a better Register type.
                    if (staticVal.type == ValueType::INT) {
                        state.registers[aa] = staticVal.i;
                        Logger::d("Interpreter", "[0x60] sget -> Loaded static int: " + std::to_string(staticVal.i) + " into v" + std::to_string(aa));
                    } else if (staticVal.type == ValueType::NULL_TYPE) {
                        state.registers[aa] = 0;
                        Logger::d("Interpreter", "[0x60] sget -> Loaded static null into v" + std::to_string(aa));
                    } else {
                        Logger::w("Interpreter", "[0x60] sget -> Unsupported static field type loaded into v" + std::to_string(aa));
                        state.registers[aa] = 0;
                    }
                } else {
                    Logger::w("Interpreter", "[0x60] sget -> No DexParser available to resolve field!");
                    state.registers[aa] = 0;
                }
                
                state.pc += 3;
                break;
            }
            case 0x6E:   // invoke-virtual
            case 0x6F:   // invoke-super
            case 0x70:   // invoke-direct
            case 0x71: { // invoke-static
                // Format 35c: A|G|op BBBB F|E|D|C
                uint8_t op = opcode;
                uint8_t ag = bytecode[state.pc];
                uint16_t methodIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                uint8_t fedc = bytecode[state.pc + 3];
                
                uint8_t argCount = ag >> 4;
                
                std::vector<Value> args;
                args.reserve(argCount);
                
                // Read arguments based on count.
                if (argCount > 0) args.push_back(Value::MakeInt(state.registers[fedc & 0x0F])); // C
                if (argCount > 1) args.push_back(Value::MakeInt(state.registers[(fedc >> 4) & 0x0F])); // D
                if (argCount > 2) args.push_back(Value::MakeInt(state.registers[(fedc >> 8) & 0x0F])); // E
                if (argCount > 3) args.push_back(Value::MakeInt(state.registers[(fedc >> 12) & 0x0F])); // F
                if (argCount > 4) args.push_back(Value::MakeInt(state.registers[ag & 0x0F])); // A

                // Note: First argument is `this` for non-static methods. 
                // Since our arguments are just pulling from raw uint32_t registers, they're built as INTs above.
                // We'll leave them as Ints for now since StubRegistry handles the dummy `this` fine.

                std::string methodName = currentDex ? currentDex->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                Logger::d("Interpreter", "Dispatching to stub: " + methodName);
                
                bool exceptionThrown = StubRegistry::invoke(methodName, &state, args, &state.methodReturnVal);
                if (exceptionThrown) {
                    Logger::e("Interpreter", "Exception thrown in native stub!");
                    return; // Halt
                }
                
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
