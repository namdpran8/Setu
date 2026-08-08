#include "Interpreter.h"
#include "../utils/Logger.h"
#include "../dex/MultiDexManager.h"
#include "StubRegistry.h"

Interpreter::Interpreter() {
}

Interpreter::~Interpreter() {
}

uint8_t Interpreter::safe8(const std::vector<uint8_t>& bytecode, uint32_t offset) {
    if (offset < bytecode.size()) return bytecode.at(offset);
    return 0;
}

uint16_t Interpreter::safe16(const std::vector<uint8_t>& bytecode, uint32_t offset) {
    uint16_t val = 0;
    if (offset < bytecode.size()) val |= safe8(bytecode, offset);
    if (offset + 1 < bytecode.size()) val |= (safe8(bytecode, offset + 1) << 8);
    return val;
}

uint8_t Interpreter::fetchOpcode(const std::vector<uint8_t>& bytecode, uint32_t& pc) {
    if (pc < bytecode.size()) return safe8(bytecode, pc++);
    Logger::e("Interpreter", "PC out of bounds! We ran off the end of the bytecode!");
    return 0; 
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

    std::unique_ptr<InterpreterState> statePtr = std::make_unique<InterpreterState>();
    InterpreterState& state = *statePtr; // Fresh state per execution, on heap to prevent stack overflow
    
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
                uint8_t ba = safe8(bytecode, state.pc);
                uint8_t a = ba & 0x0F;
                uint8_t b = (ba >> 4) & 0x0F;
                state.registers[a] = state.registers[b];
                Logger::d("Interpreter", "[0x01/04/07] move v" + std::to_string(a) + " = v" + std::to_string(b));
                state.pc += 1; // Format 12x is 2 bytes
                break;
            }
            case 0x03:   // move/16
            case 0x06:   // move-wide/16
            case 0x09: { // move-object/16
                // Format 32x: ØØ|op AAAA BBBB -> byte 1 is padding!
                uint16_t aaaa = safe16(bytecode, state.pc + 1);
                uint16_t bbbb = safe16(bytecode, state.pc + 3);
                state.registers[aaaa] = state.registers[bbbb];
                Logger::d("Interpreter", "[0x03/06/09] move/16 v" + std::to_string(aaaa) + " = v" + std::to_string(bbbb));
                state.pc += 5; // Format 32x is 6 bytes
                break;
            }
            case 0x02:   // move/from16
            case 0x05:   // move-wide/from16
            case 0x08: { // move-object/from16
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t bbbb = safe16(bytecode, state.pc + 1);
                state.registers[aa] = state.registers[bbbb];
                Logger::d("Interpreter", "[0x02/05/08] move/from16 v" + std::to_string(aa) + " = v" + std::to_string(bbbb));
                state.pc += 3; // Format 22x is 4 bytes
                break;
            }
            case 0x0E: { // return-void
                Logger::d("Interpreter", "[0x0E] return-void -> Exiting method");
                isRunning = false;
                break;
            }
            case 0x0A:   // move-result
            case 0x0B:   // move-result-wide
            case 0x0C: { // move-result-object
                uint8_t aa = safe8(bytecode, state.pc);
                state.registers[aa] = state.methodReturnVal;
                Logger::d("Interpreter", "[0x0A/0B/0C] move-result-* -> v" + std::to_string(aa));
                state.pc += 1;
                break;
            }
            case 0x1F: { // check-cast
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                std::string className = currentDex ? currentDex->getTypeString(typeIdx) : "";
                Logger::d("Interpreter", "[0x1F] check-cast v" + std::to_string(aa) + " to " + className);
                // In our simple interpreter, we don't throw ClassCastException
                state.pc += 3; // Format 21c is 4 bytes
                break;
            }
            case 0x22: { // new-instance
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                
                std::string className = currentDex ? currentDex->getTypeString(typeIdx) : "";
                Logger::d("Interpreter", "[0x22] new-instance " + className + " into v" + std::to_string(aa));
                
                InterpreterObject* obj = new InterpreterObject();
                obj->className = className;
                state.registers[aa] = Value::MakeObject(obj);
                
                state.pc += 3; // Format 21c is 4 bytes, 1 byte consumed by fetchOpcode, skip 3
                break;
            }
            case 0x28: { // goto
                int8_t aa = (int8_t)safe8(bytecode, state.pc);
                Logger::d("Interpreter", "[0x28] goto " + std::to_string(aa));
                state.pc = (state.pc - 1) + (aa * 2);
                break;
            }
            case 0x1D: // monitor-enter
            case 0x1E: { // monitor-exit
                uint8_t aa = safe8(bytecode, state.pc);
                Logger::d("Interpreter", "[0x1D/1E] monitor-* v" + std::to_string(aa));
                state.pc += 1;
                break;
            }
            case 0x27: { // throw
                uint8_t aa = safe8(bytecode, state.pc);
                Logger::w("Interpreter", "[0x27] throw v" + std::to_string(aa) + " (UNIMPLEMENTED - Exiting method to prevent crash)");
                isRunning = false;
                break;
            }
            case 0x0F: // return
            case 0x10: // return-wide
            case 0x11: { // return-object
                uint8_t aa = safe8(bytecode, state.pc);
                state.methodReturnVal = state.registers[aa];
                Logger::d("Interpreter", "[0x0F..11] return v" + std::to_string(aa));
                isRunning = false;
                break;
            }
            case 0x15: { // const/high16 vAA, #+BBBB0000
                uint8_t aa = safe8(bytecode, state.pc);
                int32_t bbbb = safe16(bytecode, state.pc + 1);
                state.registers[aa] = Value::MakeInt(bbbb << 16);
                Logger::d("Interpreter", "[0x15] const/high16 v" + std::to_string(aa) + " = " + std::to_string(bbbb << 16));
                state.pc += 3;
                break;
            }
            case 0x12: { // const/4 vA, #+B
                uint8_t ba = safe8(bytecode, state.pc);
                uint8_t a = ba & 0x0F;
                int8_t b = (ba >> 4) & 0x0F;
                if (b & 0x08) b |= 0xF0; // Sign extend
                state.registers[a] = Value::MakeInt(b);
                Logger::d("Interpreter", "[0x12] const/4 v" + std::to_string(a) + " = " + std::to_string((int)b));
                state.pc += 1;
                break;
            }
            case 0x13: { // const/16 vAA, #+BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int16_t bbbb = safe16(bytecode, state.pc + 1);
                state.registers[aa] = Value::MakeInt(bbbb);
                Logger::d("Interpreter", "[0x13] const/16 v" + std::to_string(aa) + " = " + std::to_string(bbbb));
                state.pc += 3;
                break;
            }
            case 0x14: { // const vAA, #+BBBBBBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int32_t bbbb = safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24);
                state.registers[aa] = Value::MakeInt(bbbb);
                Logger::d("Interpreter", "[0x14] const v" + std::to_string(aa) + " = " + std::to_string(bbbb));
                state.pc += 5;
                break;
            }
            case 0x2D: { // cmp-long
                uint8_t cc = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t aa = safe8(bytecode, state.pc + 2);
                // Format 23x: AA|op CC|BB -> byte1=AA, byte2=BB, byte3=CC. Wait, actually byte 0 is op, byte 1 is AA, byte 2 is BB, byte 3 is CC.
                // In fetchOpcode we consumed op. So state.pc points to AA.
                aa = safe8(bytecode, state.pc);
                bb = safe8(bytecode, state.pc + 1);
                cc = safe8(bytecode, state.pc + 2);
                
                long long valB = state.registers[bb].i;
                long long valC = state.registers[cc].i;
                int result = (valB == valC) ? 0 : ((valB > valC) ? 1 : -1);
                state.registers[aa] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0x2D] cmp-long v" + std::to_string(aa) + " = v" + std::to_string(bb) + " vs v" + std::to_string(cc));
                state.pc += 3; // Format 23x is 4 bytes
                break;
            }
            case 0x2E: case 0x2F: case 0x30: case 0x31: { // cmp-float / cmp-double
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                state.registers[aa] = Value::MakeInt(0); // stub
                state.pc += 3;
                break;
            }
            case 0x32: case 0x33: case 0x34: case 0x35:
            case 0x36: case 0x37: { // if-* vA, vB, +CCCC
                uint8_t ba = safe8(bytecode, state.pc);
                uint8_t a = ba & 0x0F;
                uint8_t b = (ba >> 4) & 0x0F;
                int16_t offset = (int16_t)(safe16(bytecode, state.pc + 1));
                
                long long valA = state.registers[a].i;
                long long valB = state.registers[b].i;
                bool condition = false;
                switch (opcode) {
                    case 0x32: condition = (valA == valB); break; // if-eq
                    case 0x33: condition = (valA != valB); break; // if-ne
                    case 0x34: condition = (valA < valB); break;  // if-lt
                    case 0x35: condition = (valA >= valB); break; // if-ge
                    case 0x36: condition = (valA > valB); break;  // if-gt
                    case 0x37: condition = (valA <= valB); break; // if-le
                }
                
                Logger::d("Interpreter", "[0x32..37] if-* v" + std::to_string(a) + ", v" + std::to_string(b) + " -> " + std::to_string(offset));
                if (condition) {
                    state.pc = (state.pc - 1) + (offset * 2);
                } else {
                    state.pc += 3; // Format 22t is 4 bytes
                }
                break;
            }
            case 0x38: case 0x39: case 0x3A: case 0x3B:
            case 0x3C: case 0x3D: { // if-*z vAA, +BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int16_t offset = (int16_t)(safe16(bytecode, state.pc + 1));
                
                long long val = state.registers[aa].i;
                bool condition = false;
                switch (opcode) {
                    case 0x38: condition = (val == 0); break; // if-eqz
                    case 0x39: condition = (val != 0); break; // if-nez
                    case 0x3A: condition = (val < 0); break;  // if-ltz
                    case 0x3B: condition = (val >= 0); break; // if-gez
                    case 0x3C: condition = (val > 0); break;  // if-gtz
                    case 0x3D: condition = (val <= 0); break; // if-lez
                }
                
                Logger::d("Interpreter", "[0x38..3D] if-*z v" + std::to_string(aa) + " -> " + std::to_string(offset));
                if (condition) {
                    state.pc = (state.pc - 1) + (offset * 2);
                } else {
                    state.pc += 3; // Format 21t is 4 bytes
                }
                break;
            }
            case 0x1A: { // const-string vAA, string@BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t stringIdx = safe16(bytecode, state.pc + 1);
                if (currentDex) {
                    InterpreterObject* strObj = new InterpreterObject();
                    strObj->className = "java.lang.String";
                    state.registers[aa] = Value::MakeObject(strObj);
                    Logger::d("Interpreter", "[0x1A] const-string v" + std::to_string(aa) + " = string@" + std::to_string(stringIdx));
                }
                state.pc += 3;
                break;
            }
            case 0x20: { // instance-of vA, vB, type@CCCC
                uint8_t a = safe8(bytecode, state.pc) & 0x0F;
                uint8_t b = (safe8(bytecode, state.pc) >> 4) & 0x0F;
                uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                state.registers[a] = Value::MakeInt(0); // stub to false
                Logger::d("Interpreter", "[0x20] instance-of v" + std::to_string(a) + " = v" + std::to_string(b) + " instanceof type@" + std::to_string(typeIdx));
                state.pc += 3;
                break;
            }
            case 0x2B: // packed-switch
            case 0x2C: { // sparse-switch
                uint8_t aa = safe8(bytecode, state.pc);
                Logger::d("Interpreter", "[0x2B/2C] switch v" + std::to_string(aa) + " (UNIMPLEMENTED - falling through)");
                state.pc += 5; // Format 31t is 6 bytes
                break;
            }
            case 0x1C: { // const-class vAA, type@BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t typeIdx = safe16(bytecode, state.pc + 1);
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
            case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4A: { // aget-*
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                // vAA = vBB[vCC] - Mocking arrays for now
                state.registers[aa] = Value::MakeNull();
                Logger::d("Interpreter", "[0x44..4A] aget-* v" + std::to_string(aa) + " = v" + std::to_string(bb) + "[v" + std::to_string(cc) + "]");
                state.pc += 3; // Format 23x is 4 bytes (1 byte consumed by fetchOpcode)
                break;
            }
            case 0x4B: case 0x4C: case 0x4D: case 0x4E:
            case 0x4F: case 0x50: case 0x51: { // aput-*
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                Logger::d("Interpreter", "[0x4B..51] aput-* v" + std::to_string(bb) + "[v" + std::to_string(cc) + "] = v" + std::to_string(aa));
                state.pc += 3; // Format 23x is 4 bytes
                break;
            }
            case 0x52: case 0x53: case 0x54: case 0x55:
            case 0x56: case 0x57: case 0x58: { // iget-* vA, vB, field@CCCC
                uint8_t a = safe8(bytecode, state.pc) & 0x0F;
                uint8_t b = (safe8(bytecode, state.pc) >> 4) & 0x0F;
                uint16_t fieldIdx = safe16(bytecode, state.pc + 1);
                
                Value& objVal = state.registers[b];
                if (objVal.type == ValueType::OBJECT && objVal.obj) {
                    InterpreterObject* instance = (InterpreterObject*)objVal.obj;
                    std::string fieldName = "field_" + std::to_string(fieldIdx);
                    if (instance->fields.count(fieldName)) {
                        state.registers[a] = instance->fields[fieldName];
                    } else {
                        state.registers[a] = Value::MakeNull();
                    }
                    Logger::d("Interpreter", "[0x52..58] iget-* v" + std::to_string(a) + ", v" + std::to_string(b) + " -> " + fieldName);
                } else {
                    Logger::w("Interpreter", "[0x52..58] iget-* failed: vB is null or not object");
                    state.registers[a] = Value::MakeNull();
                }
                state.pc += 3;
                break;
            }
            case 0x59: case 0x5A: case 0x5B: case 0x5C:
            case 0x5D: case 0x5E: case 0x5F: { // iput-* vA, vB, field@CCCC
                uint8_t a = safe8(bytecode, state.pc) & 0x0F;
                uint8_t b = (safe8(bytecode, state.pc) >> 4) & 0x0F;
                uint16_t fieldIdx = safe16(bytecode, state.pc + 1);
                
                Value& valToStore = state.registers[a];
                Value& objVal = state.registers[b];
                
                if (objVal.type == ValueType::OBJECT && objVal.obj) {
                    InterpreterObject* instance = (InterpreterObject*)objVal.obj;
                    std::string fieldName = "field_" + std::to_string(fieldIdx);
                    instance->fields[fieldName] = valToStore;
                    Logger::d("Interpreter", "[0x59..5F] iput-* v" + std::to_string(a) + ", v" + std::to_string(b) + " -> " + fieldName);
                } else {
                    Logger::w("Interpreter", "[0x59..5F] iput-* failed: vB is null or not object");
                }
                state.pc += 3;
                break;
            }
            case 0x60: case 0x61: case 0x62: case 0x63:
            case 0x64: case 0x65: case 0x66: { // sget-*
                // Format 21c: AA|op BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t fieldIdx = safe16(bytecode, state.pc + 1);
                
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
                        Logger::d("Interpreter", "[0x60..66] sget-* -> Loaded static into v" + std::to_string(aa));
                    } else {
                        Logger::w("Interpreter", "[0x60..66] sget-* -> Unsupported static field type loaded into v" + std::to_string(aa));
                        state.registers[aa] = Value::MakeNull();
                    }
                } else {
                    Logger::w("Interpreter", "[0x60..66] sget-* -> No DexParser available to resolve field!");
                    state.registers[aa] = Value::MakeNull();
                }
                
                state.pc += 3;
                break;
            }
            case 0x67: case 0x68: case 0x69: case 0x6A:
            case 0x6B: case 0x6C: case 0x6D: { // sput-*
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t fieldIdx = safe16(bytecode, state.pc + 1);
                Logger::d("Interpreter", "[0x67..6D] sput-* from v" + std::to_string(aa));
                state.pc += 3;
                break;
            }
            case 0x6E:   // invoke-virtual
            case 0x6F:   // invoke-super
            case 0x70:   // invoke-direct
            case 0x71:   // invoke-static
            case 0x72:   // invoke-interface
            case 0x74:   // invoke-virtual/range
            case 0x75:   // invoke-super/range
            case 0x76:   // invoke-direct/range
            case 0x77:   // invoke-static/range
            case 0x78: { // invoke-interface/range
                bool isRange = (opcode >= 0x74 && opcode <= 0x78);
                uint8_t aa = safe8(bytecode, state.pc);
                uint16_t methodIdx = safe16(bytecode, state.pc + 1);
                
                uint8_t argCount = isRange ? aa : (aa >> 4);
                
                std::vector<Value> args;
                args.reserve(argCount);
                
                if (isRange) {
                    // Format 3rc: AA|op BBBB CCCC
                    uint16_t cccc = safe16(bytecode, state.pc + 3);
                    for (int i = 0; i < argCount; ++i) {
                        args.push_back(state.registers[cccc + i]);
                    }
                } else {
                    // Format 35c: A|G|op BBBB F|E|D|C
                    uint16_t fedc = safe16(bytecode, state.pc + 3);
                    if (argCount > 0) args.push_back(state.registers[fedc & 0x0F]); // C
                    if (argCount > 1) args.push_back(state.registers[(fedc >> 4) & 0x0F]); // D
                    if (argCount > 2) args.push_back(state.registers[(fedc >> 8) & 0x0F]); // E
                    if (argCount > 3) args.push_back(state.registers[(fedc >> 12) & 0x0F]); // F
                    if (argCount > 4) args.push_back(state.registers[aa & 0x0F]); // A
                }

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
                    if (StubRegistry::isStubbed(methodSig)) {
                        executedBytecode = StubRegistry::invoke(methodSig, &state, args, &state.methodReturnVal);
                    } else {
                        auto [bcResult, bcDex] = multiDexManager->getMethodBytecode(methodSig);
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
                    snprintf(buf, sizeof(buf), "%02X ", safe8(bytecode, i));
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
