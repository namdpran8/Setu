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

Value Interpreter::executeMethod(const std::vector<uint8_t>& bytecode, 
                               const DexParser* currentDex, 
                               const MultiDexManager* multiDexManager,
                               const std::vector<Value>& args,
                               uint16_t registers_size,
                               uint16_t ins_size) {
    if (bytecode.empty()) {
        Logger::w("Interpreter", "Cannot execute empty bytecode!");
        return Value::MakeNull();
    }

    InterpreterState state; // Fresh state per execution
    
    // Seed parameters into the highest registers
    if (ins_size > 0 && registers_size >= ins_size) {
        int firstParamRegister = registers_size - ins_size;
        for (size_t i = 0; i < args.size() && i < ins_size; ++i) {
            state.registers[firstParamRegister + i] = args[i];
        }
    }

    Logger::i("Interpreter", "Starting bytecode execution loop...");
    
    bool isRunning = true;
    while (isRunning && state.pc < bytecode.size()) {
        // 1. Fetch the opcode
        uint8_t opcode = fetchOpcode(bytecode, state.pc);
        
        // 2. Decode and Execute
        switch (opcode) {
            case 0x00: { // nop
                Logger::d("Interpreter", "[0x00] nop");
                state.pc += 1; // Format 10x is 2 bytes (1 read by fetchOpcode)
                break;
            }
            case 0x01:   // move
            case 0x04:   // move-wide
            case 0x07: { // move-object
                uint8_t ba = bytecode[state.pc];
                uint8_t a = ba & 0x0F;
                uint8_t b = (ba >> 4) & 0x0F;
                state.registers[a] = state.registers[b];
                Logger::d("Interpreter", "[0x01/04/07] move v" + std::to_string(a) + " = v" + std::to_string(b));
                state.pc += 1; // Format 12x is 2 bytes
                break;
            }
            case 0x0E: { // return-void
                Logger::d("Interpreter", "[0x0E] return-void -> Exiting method");
                isRunning = false;
                break;
            }
            case 0x0C: { // move-result-object
                uint8_t aa = bytecode[state.pc];
                state.registers[aa] = state.methodReturnVal;
                Logger::d("Interpreter", "[0x0C] move-result-object -> v" + std::to_string(aa));
                state.pc += 1;
                break;
            }
            case 0x1F: { // check-cast
                uint8_t aa = bytecode[state.pc];
                uint16_t typeIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                std::string className = currentDex ? currentDex->getTypeString(typeIdx) : "";
                Logger::d("Interpreter", "[0x1F] check-cast v" + std::to_string(aa) + " to " + className);
                // In our simple interpreter, we don't throw ClassCastException
                state.pc += 3; // Format 21c is 4 bytes
                break;
            }
            case 0x22: { // new-instance
                uint8_t aa = bytecode[state.pc];
                uint16_t typeIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                
                std::string className = currentDex ? currentDex->getTypeString(typeIdx) : "";
                Logger::d("Interpreter", "[0x22] new-instance " + className + " into v" + std::to_string(aa));
                
                InterpreterObject* obj = new InterpreterObject();
                obj->className = className;
                state.registers[aa] = Value::MakeObject(obj);
                
                state.pc += 3; // Format 21c is 4 bytes, 1 byte consumed by fetchOpcode, skip 3
                break;
            }
            case 0x28: { // goto
                int8_t aa = (int8_t)bytecode[state.pc];
                Logger::d("Interpreter", "[0x28] goto " + std::to_string(aa));
                state.pc = (state.pc - 1) + (aa * 2);
                break;
            }
            case 0x31: { // cmp-long
                uint8_t aa = bytecode[state.pc];
                uint8_t ccbb = bytecode[state.pc + 1]; // Format 23x: op vAA, vBB, vCC -> op=byte0, AA=byte1, CC|BB=byte2
                uint8_t bb = ccbb & 0x0F;
                uint8_t cc = (ccbb >> 4) & 0x0F;
                // We'll just compare the Value::i as 64-bit longs
                long long valB = state.registers[bb].i;
                long long valC = state.registers[cc].i;
                int result = (valB == valC) ? 0 : ((valB > valC) ? 1 : -1);
                state.registers[aa] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0x31] cmp-long v" + std::to_string(aa) + " = v" + std::to_string(bb) + " vs v" + std::to_string(cc));
                state.pc += 3; // Format 23x is 4 bytes
                break;
            }
            case 0x38: { // if-eqz
                uint8_t aa = bytecode[state.pc];
                int16_t offset = (int16_t)(bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8));
                Logger::d("Interpreter", "[0x38] if-eqz v" + std::to_string(aa) + " -> " + std::to_string(offset));
                if (state.registers[aa].i == 0) {
                    state.pc = (state.pc - 1) + (offset * 2);
                } else {
                    state.pc += 3; // Format 21t is 4 bytes
                }
                break;
            }
            case 0x39: { // if-nez
                uint8_t aa = bytecode[state.pc];
                int16_t offset = (int16_t)(bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8));
                Logger::d("Interpreter", "[0x39] if-nez v" + std::to_string(aa) + " -> " + std::to_string(offset));
                if (state.registers[aa].i != 0) {
                    state.pc = (state.pc - 1) + (offset * 2);
                } else {
                    state.pc += 3; // Format 21t is 4 bytes
                }
                break;
            }
            case 0x13: { // const/16 vAA, #+BBBB
                uint8_t aa = bytecode[state.pc];
                int16_t lit = (int16_t)(bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8));
                state.registers[aa] = Value::MakeInt(lit);
                Logger::d("Interpreter", "[0x13] const/16 v" + std::to_string(aa) + " = " + std::to_string(lit));
                state.pc += 3;
                break;
            }
            case 0x1A: { // const-string vAA, string@BBBB
                uint8_t aa = bytecode[state.pc];
                uint16_t stringIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                if (currentDex) {
                    InterpreterObject* strObj = new InterpreterObject();
                    strObj->className = "java.lang.String";
                    state.registers[aa] = Value::MakeObject(strObj);
                    Logger::d("Interpreter", "[0x1A] const-string v" + std::to_string(aa) + " = string@" + std::to_string(stringIdx));
                }
                state.pc += 3;
                break;
            }
            case 0x1C: { // const-class vAA, type@BBBB
                uint8_t aa = bytecode[state.pc];
                uint16_t typeIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                if (currentDex) {
                    std::string className = currentDex->getTypeString(typeIdx);
                    InterpreterObject* classObj = new InterpreterObject();
                    classObj->className = "java.lang.Class";
                    
                    // Store the target class name in a pseudo-field so we can read it later
                    InterpreterObject* targetClassStrObj = new InterpreterObject();
                    targetClassStrObj->className = className; // Use className to store string for now
                    classObj->fields["targetClass"] = Value::MakeObject(targetClassStrObj);
                    
                    state.registers[aa] = Value::MakeObject(classObj);
                    Logger::d("Interpreter", "[0x1C] const-class v" + std::to_string(aa) + " = " + className);
                }
                state.pc += 3;
                break;
            }
            case 0x54: { // iget-object vA, vB, field@CCCC
                uint8_t a = bytecode[state.pc] & 0x0F;
                uint8_t b = (bytecode[state.pc] >> 4) & 0x0F;
                uint16_t fieldIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                
                Value& objVal = state.registers[b];
                if (objVal.type == ValueType::OBJECT && objVal.obj) {
                    InterpreterObject* instance = (InterpreterObject*)objVal.obj;
                    std::string fieldName = "field_" + std::to_string(fieldIdx);
                    if (instance->fields.count(fieldName)) {
                        state.registers[a] = instance->fields[fieldName];
                    } else {
                        state.registers[a] = Value::MakeNull();
                    }
                    Logger::d("Interpreter", "[0x54] iget-object v" + std::to_string(a) + ", v" + std::to_string(b) + " -> " + fieldName);
                } else {
                    Logger::w("Interpreter", "[0x54] iget-object failed: vB is null or not object");
                    state.registers[a] = Value::MakeNull();
                }
                state.pc += 3;
                break;
            }
            case 0x5B: { // iput-object vA, vB, field@CCCC
                uint8_t a = bytecode[state.pc] & 0x0F;
                uint8_t b = (bytecode[state.pc] >> 4) & 0x0F;
                uint16_t fieldIdx = bytecode[state.pc + 1] | (bytecode[state.pc + 2] << 8);
                
                Value& valToStore = state.registers[a];
                Value& objVal = state.registers[b];
                
                if (objVal.type == ValueType::OBJECT && objVal.obj) {
                    InterpreterObject* instance = (InterpreterObject*)objVal.obj;
                    std::string fieldName = "field_" + std::to_string(fieldIdx);
                    instance->fields[fieldName] = valToStore;
                    Logger::d("Interpreter", "[0x5B] iput-object v" + std::to_string(a) + ", v" + std::to_string(b) + " -> " + fieldName);
                } else {
                    Logger::w("Interpreter", "[0x5B] iput-object failed: vB is null or not object");
                }
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
                    
                    if (staticVal.type == ValueType::INT || staticVal.type == ValueType::OBJECT || staticVal.type == ValueType::NULL_TYPE) {
                        state.registers[aa] = staticVal;
                        Logger::d("Interpreter", "[0x60] sget -> Loaded static into v" + std::to_string(aa));
                    } else {
                        Logger::w("Interpreter", "[0x60] sget -> Unsupported static field type loaded into v" + std::to_string(aa));
                        state.registers[aa] = Value::MakeNull();
                    }
                } else {
                    Logger::w("Interpreter", "[0x60] sget -> No DexParser available to resolve field!");
                    state.registers[aa] = Value::MakeNull();
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
                uint16_t fedc = bytecode[state.pc + 3] | (bytecode[state.pc + 4] << 8);
                
                uint8_t argCount = ag >> 4;
                
                std::vector<Value> args;
                args.reserve(argCount);
                
                // Read arguments based on count.
                if (argCount > 0) args.push_back(state.registers[fedc & 0x0F]); // C
                if (argCount > 1) args.push_back(state.registers[(fedc >> 4) & 0x0F]); // D
                if (argCount > 2) args.push_back(state.registers[(fedc >> 8) & 0x0F]); // E
                if (argCount > 3) args.push_back(state.registers[(fedc >> 12) & 0x0F]); // F
                if (argCount > 4) args.push_back(state.registers[ag & 0x0F]); // A

                std::string methodSig = currentDex ? currentDex->getMethodSignature(methodIdx) : std::to_string(methodIdx);
                
                // 1. Try to find the bytecode in the DEX file
                // getMethodSignature returns "Lclass;->method(Lparams;)V"
                // getMethodBytecode expects (className, methodName)
                // We need to parse methodSig to extract className and methodName
                // Format: Lcom/pranshu/test1/MainActivity;->onCreate(Landroid/os/Bundle;)V
                size_t arrowPos = methodSig.find("->");
                size_t parenPos = methodSig.find('(');
                bool executedBytecode = false;
                
                if (arrowPos != std::string::npos && parenPos != std::string::npos && multiDexManager != nullptr) {
                    std::string clsName = methodSig.substr(0, arrowPos);
                    std::string mthName = methodSig.substr(arrowPos + 2, parenPos - (arrowPos + 2));
                    
                    if (StubRegistry::isStubbed(methodSig)) {
                        executedBytecode = StubRegistry::invoke(methodSig, &state, args, &state.methodReturnVal);
                    } else {
                        auto [bcResult, bcDex] = multiDexManager->getMethodBytecode(clsName, mthName);
                        if (!bcResult.bytecode.empty() && bcDex) {
                            Logger::d("Interpreter", "Executing DEX bytecode for: " + methodSig);
                            // Recursive execution!
                            Interpreter nestedVm;
                            state.methodReturnVal = nestedVm.executeMethod(bcResult.bytecode, bcDex, multiDexManager, args, bcResult.registers_size, bcResult.ins_size);
                            executedBytecode = true;
                        }
                    }
                }
                
                // 2. If not found in DEX, dispatch to native stub
                if (!executedBytecode) {
                    Logger::d("Interpreter", "Dispatching to stub: " + methodSig);
                    bool exceptionThrown = StubRegistry::invoke(methodSig, &state, args, &state.methodReturnVal);
                    if (exceptionThrown) {
                        Logger::e("Interpreter", "Exception thrown in native stub!");
                        return Value::MakeNull(); // Halt
                    }
                }
                
                state.pc += 5;
                break;
            }
            default: {
                // Unimplemented opcode!
                char hexBuf[16];
                snprintf(hexBuf, sizeof(hexBuf), "0x%02X", opcode);
                Logger::e("Interpreter", "UNIMPLEMENTED OPCODE: " + std::string(hexBuf) + " in method.");
                
                // Dump the raw hex of the code item for debugging
                std::string hexDump = "";
                for (size_t i = 0; i < bytecode.size() && i < 64; i++) {
                    char buf[16];
                    snprintf(buf, sizeof(buf), "%02X ", bytecode[i]);
                    hexDump += buf;
                }
                Logger::e("Interpreter", "Method bytecode prefix: " + hexDump);
                
                isRunning = false;
                break;
            }
        }
    }
    
    Logger::i("Interpreter", "Execution finished.");
    return state.methodReturnVal;
}
