#include "Interpreter.h"
#include "../utils/Logger.h"
#include "../dex/MultiDexManager.h"
#include "StubRegistry.h"
#include <cmath>

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
            case 0x0D: { // move-exception
                uint8_t aa = safe8(bytecode, state.pc);
                state.registers[aa] = Value::MakeNull(); // exception_register not fully implemented
                Logger::d("Interpreter", "[0x0D] move-exception -> v" + std::to_string(aa));
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
            case 0x29: { // goto/16
                // format 20t: ØØ|op AAAA
                int16_t aaaa = (int16_t)safe16(bytecode, state.pc + 1);
                Logger::d("Interpreter", "[0x29] goto/16 " + std::to_string(aaaa));
                state.pc = (state.pc - 1) + (aaaa * 2);
                break;
            }
            case 0x2A: { // goto/32
                // format 30t: ØØ|op AAAAAAAA
                int32_t aaaaaaaa = (int32_t)(safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24));
                Logger::d("Interpreter", "[0x2A] goto/32 " + std::to_string(aaaaaaaa));
                state.pc = (state.pc - 1) + (aaaaaaaa * 2);
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
            case 0x16: { // const-wide/16 vAA, #+BBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int16_t bbbb = safe16(bytecode, state.pc + 1);
                // Mocking wide values as two 32-bit registers, ignoring upper 32-bits for now
                state.registers[aa] = Value::MakeInt(bbbb);
                state.registers[aa + 1] = Value::MakeInt(bbbb < 0 ? -1 : 0);
                Logger::d("Interpreter", "[0x16] const-wide/16 v" + std::to_string(aa) + " = " + std::to_string(bbbb));
                state.pc += 3;
                break;
            }
            case 0x17: { // const-wide/32 vAA, #+BBBBBBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int32_t bbbb = safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24);
                state.registers[aa] = Value::MakeInt(bbbb);
                state.registers[aa + 1] = Value::MakeInt(bbbb < 0 ? -1 : 0);
                Logger::d("Interpreter", "[0x17] const-wide/32 v" + std::to_string(aa) + " = " + std::to_string(bbbb));
                state.pc += 5;
                break;
            }
            case 0x18: { // const-wide vAA, #+BBBBBBBBBBBBBBBB
                uint8_t aa = safe8(bytecode, state.pc);
                int32_t low = safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24);
                int32_t high = safe16(bytecode, state.pc + 5) | (safe8(bytecode, state.pc + 7) << 16) | (safe8(bytecode, state.pc + 8) << 24);
                state.registers[aa] = Value::MakeInt(low);
                state.registers[aa + 1] = Value::MakeInt(high);
                Logger::d("Interpreter", "[0x18] const-wide v" + std::to_string(aa) + " = " + std::to_string(low) + " (high: " + std::to_string(high) + ")");
                state.pc += 9;
                break;
            }
            case 0x19: { // const-wide/high16 vAA, #+BBBB000000000000
                uint8_t aa = safe8(bytecode, state.pc);
                int32_t bbbb = safe16(bytecode, state.pc + 1);
                state.registers[aa] = Value::MakeInt(0);
                state.registers[aa + 1] = Value::MakeInt(bbbb << 16);
                Logger::d("Interpreter", "[0x19] const-wide/high16 v" + std::to_string(aa) + " = " + std::to_string(bbbb) + "<<48");
                state.pc += 3;
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
            case 0x2E: case 0x2F: { // cmpl-float, cmpg-float
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                
                float valB = (state.registers[bb].type == ValueType::FLOAT) ? state.registers[bb].f : 0.0f;
                float valC = (state.registers[cc].type == ValueType::FLOAT) ? state.registers[cc].f : 0.0f;
                
                int result = 0;
                if (std::isnan(valB) || std::isnan(valC)) {
                    result = (opcode == 0x2E) ? -1 : 1; // 0x2E is cmpl, 0x2F is cmpg
                } else if (valB == valC) {
                    result = 0;
                } else if (valB > valC) {
                    result = 1;
                } else {
                    result = -1;
                }
                
                state.registers[aa] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0x2E..2F] cmp-float v" + std::to_string(aa) + " = v" + std::to_string(bb) + " vs v" + std::to_string(cc));
                state.pc += 3;
                break;
            }
            case 0x30: case 0x31: { // cmpl-double, cmpg-double
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                
                // treating as float for now since double spans 2 registers
                float valB = (state.registers[bb].type == ValueType::FLOAT) ? state.registers[bb].f : 0.0f;
                float valC = (state.registers[cc].type == ValueType::FLOAT) ? state.registers[cc].f : 0.0f;
                
                int result = 0;
                if (std::isnan(valB) || std::isnan(valC)) {
                    result = (opcode == 0x30) ? -1 : 1; // 0x30 is cmpl, 0x31 is cmpg
                } else if (valB == valC) {
                    result = 0;
                } else if (valB > valC) {
                    result = 1;
                } else {
                    result = -1;
                }
                
                state.registers[aa] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0x30..31] cmp-double v" + std::to_string(aa) + " = v" + std::to_string(bb) + " vs v" + std::to_string(cc));
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
                    
                    std::string text = currentDex->getString(stringIdx);
                    InterpreterObject* inner = new InterpreterObject();
                    inner->className = text;
                    strObj->fields["string_value"] = Value::MakeObject(inner);
                    
                    state.registers[aa] = Value::MakeObject(strObj);
                    Logger::d("Interpreter", "[0x1A] const-string v" + std::to_string(aa) + " = string@" + std::to_string(stringIdx) + " (" + text + ")");
                }
                state.pc += 3;
                break;
            }
            case 0x1B: { // const-string/jumbo vAA, string@BBBBBBBB
                uint8_t aa = safe8(bytecode, state.pc);
                uint32_t stringIdx = safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24);
                if (currentDex) {
                    InterpreterObject* strObj = new InterpreterObject();
                    strObj->className = "java.lang.String";
                    
                    std::string text = currentDex->getString(stringIdx);
                    InterpreterObject* inner = new InterpreterObject();
                    inner->className = text;
                    strObj->fields["string_value"] = Value::MakeObject(inner);
                    
                    state.registers[aa] = Value::MakeObject(strObj);
                    Logger::d("Interpreter", "[0x1B] const-string/jumbo v" + std::to_string(aa) + " = string@" + std::to_string(stringIdx) + " (" + text + ")");
                }
                state.pc += 5;
                break;
            }
            case 0x20: { // instance-of vA, vB, type@CCCC
                uint8_t a = safe8(bytecode, state.pc) & 0x0F;
                uint8_t b = (safe8(bytecode, state.pc) >> 4) & 0x0F;
                uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                
                int result = 0;
                if (state.registers[b].type == ValueType::OBJECT && state.registers[b].obj != nullptr) {
                    result = 1; // For now, assume true for all non-null objects to let stubs pass
                }
                
                state.registers[a] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0x20] instance-of v" + std::to_string(a) + " = v" + std::to_string(b) + " instanceof type@" + std::to_string(typeIdx) + " -> " + std::to_string(result));
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
            case 0x21: { // array-length vA, vB
                  uint8_t ab = safe8(bytecode, state.pc);
                  uint8_t a = ab & 0x0F;
                  uint8_t b = (ab >> 4) & 0x0F;
                  
                  if (state.registers[b].type != ValueType::ARRAY) {
                      state.registers[a] = Value::MakeInt(0);
                      Logger::w("Interpreter", "[0x21] array-length failed: v" + std::to_string(b) + " is not an array");
                  } else {
                      ArrayObject* arr = static_cast<ArrayObject*>(state.registers[b].obj);
                      state.registers[a] = Value::MakeInt((int32_t)arr->elements.size());
                      Logger::d("Interpreter", "[0x21] array-length v" + std::to_string(a) + " = length(v" + std::to_string(b) + ") -> " + std::to_string(arr->elements.size()));
                  }
                  state.pc += 1;
                  break;
              }
              case 0x23: { // new-array vA, vB, type@CCCC
                  uint8_t ab = safe8(bytecode, state.pc);
                  uint8_t a = ab & 0x0F;
                  uint8_t b = (ab >> 4) & 0x0F;
                  uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                  
                  int32_t size = state.registers[b].i;
                  ArrayObject* arr = new ArrayObject();
                  arr->elementTypeIndex = typeIdx;
                  arr->elements.resize(size > 0 ? size : 0);
                  
                  state.registers[a] = Value::MakeArray(arr);
                  Logger::d("Interpreter", "[0x23] new-array v" + std::to_string(a) + ", size v" + std::to_string(b) + "=" + std::to_string(size) + " type@" + std::to_string(typeIdx));
                  state.pc += 3;
                  break;
              }
              case 0x24: { // filled-new-array {vD, vE, vF, vG, vA}, type@CCCC
                  uint8_t ab = safe8(bytecode, state.pc);
                  uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                  // Not fully implementing array payload extraction yet, just stubbing the return register so it doesn't crash
                  ArrayObject* arr = new ArrayObject();
                  arr->elementTypeIndex = typeIdx;
                  state.methodReturnVal = Value::MakeArray(arr); // result goes to special register
                  Logger::d("Interpreter", "[0x24] filled-new-array type@" + std::to_string(typeIdx));
                  state.pc += 5;
                  break;
              }
              case 0x25: { // filled-new-array/range
                  uint8_t aa = safe8(bytecode, state.pc);
                  uint16_t typeIdx = safe16(bytecode, state.pc + 1);
                  ArrayObject* arr = new ArrayObject();
                  arr->elementTypeIndex = typeIdx;
                  state.methodReturnVal = Value::MakeArray(arr);
                  Logger::d("Interpreter", "[0x25] filled-new-array/range type@" + std::to_string(typeIdx));
                  state.pc += 5;
                  break;
              }
              case 0x26: { // fill-array-data vAA, +BBBBBBBB
                  uint8_t aa = safe8(bytecode, state.pc);
                  int32_t offset = (int32_t)(safe16(bytecode, state.pc + 1) | (safe8(bytecode, state.pc + 3) << 16) | (safe8(bytecode, state.pc + 4) << 24));
                  Logger::d("Interpreter", "[0x26] fill-array-data v" + std::to_string(aa) + " offset=" + std::to_string(offset));
                  // We skip actual population since it requires reading payload table
                  state.pc += 5;
                  break;
              }
              case 0x44: case 0x45: case 0x46: case 0x47:
              case 0x48: case 0x49: case 0x4A: { // aget-* vAA, vBB, vCC
                  uint8_t aa = safe8(bytecode, state.pc);
                  uint8_t bb = safe8(bytecode, state.pc + 1);
                  uint8_t cc = safe8(bytecode, state.pc + 2);
                  
                  if (state.registers[bb].type != ValueType::ARRAY) {
                      state.registers[aa] = Value::MakeNull();
                      Logger::w("Interpreter", "[0x44..4A] aget-* failed: v" + std::to_string(bb) + " is not an array");
                  } else {
                      ArrayObject* arr = static_cast<ArrayObject*>(state.registers[bb].obj);
                      int32_t index = state.registers[cc].i;
                      if (index >= 0 && index < arr->elements.size()) {
                          state.registers[aa] = arr->elements[index];
                          Logger::d("Interpreter", "[0x44..4A] aget-* v" + std::to_string(aa) + " = v" + std::to_string(bb) + "[v" + std::to_string(cc) + " (" + std::to_string(index) + ")]");
                      } else {
                          state.registers[aa] = Value::MakeNull();
                          Logger::w("Interpreter", "[0x44..4A] aget-* Out of bounds! index=" + std::to_string(index) + " size=" + std::to_string(arr->elements.size()));
                      }
                  }
                  state.pc += 3;
                  break;
              }
              case 0x4B: case 0x4C: case 0x4D: case 0x4E:
              case 0x4F: case 0x50: case 0x51: { // aput-* vAA, vBB, vCC
                  uint8_t aa = safe8(bytecode, state.pc);
                  uint8_t bb = safe8(bytecode, state.pc + 1);
                  uint8_t cc = safe8(bytecode, state.pc + 2);
                  
                  if (state.registers[bb].type != ValueType::ARRAY) {
                      Logger::w("Interpreter", "[0x4B..51] aput-* failed: v" + std::to_string(bb) + " is not an array");
                  } else {
                      ArrayObject* arr = static_cast<ArrayObject*>(state.registers[bb].obj);
                      int32_t index = state.registers[cc].i;
                      if (index >= 0 && index < arr->elements.size()) {
                          arr->elements[index] = state.registers[aa];
                          Logger::d("Interpreter", "[0x4B..51] aput-* v" + std::to_string(bb) + "[v" + std::to_string(cc) + " (" + std::to_string(index) + ")] = v" + std::to_string(aa));
                      } else {
                          Logger::w("Interpreter", "[0x4B..51] aput-* Out of bounds! index=" + std::to_string(index) + " size=" + std::to_string(arr->elements.size()));
                      }
                  }
                  state.pc += 3;
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
                    Logger::w("Interpreter", "[0x52..58] iget-* failed: v" + std::to_string(b) + " is null or not object. Type=" + std::to_string((int)objVal.type));
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
                    Logger::w("Interpreter", "[0x59..5F] iput-* failed: v" + std::to_string(b) + " is null or not object. Type=" + std::to_string((int)objVal.type));
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
            case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: case 0x80: case 0x81:
            case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
            case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: { // unary ops (12x)
                uint8_t ab = safe8(bytecode, state.pc);
                uint8_t a = ab & 0x0F;
                uint8_t b = (ab >> 4) & 0x0F;
                
                if (opcode == 0x7B || opcode == 0x7D) { // neg-int, neg-long
                    state.registers[a] = Value::MakeInt(-state.registers[b].i);
                } else if (opcode == 0x7C || opcode == 0x7E) { // not-int, not-long
                    state.registers[a] = Value::MakeInt(~state.registers[b].i);
                } else if (opcode == 0x7F || opcode == 0x80) { // neg-float, neg-double
                    state.registers[a] = Value::MakeFloat(-state.registers[b].f);
                } else if (opcode == 0x81 || opcode == 0x84) { // int-to-long, long-to-int
                    state.registers[a] = state.registers[b];
                } else if (opcode == 0x82 || opcode == 0x85) { // int-to-float, long-to-float
                    state.registers[a] = Value::MakeFloat((float)state.registers[b].i);
                } else if (opcode == 0x83 || opcode == 0x86) { // int-to-double, long-to-double
                    state.registers[a] = Value::MakeFloat((float)state.registers[b].i);
                } else if (opcode == 0x87 || opcode == 0x88) { // float-to-int, float-to-long
                    state.registers[a] = Value::MakeInt((int32_t)state.registers[b].f);
                } else if (opcode == 0x89 || opcode == 0x8C) { // float-to-double, double-to-float
                    state.registers[a] = state.registers[b];
                } else if (opcode == 0x8A || opcode == 0x8B) { // double-to-int, double-to-long
                    state.registers[a] = Value::MakeInt((int32_t)state.registers[b].f);
                } else if (opcode == 0x8D) { // int-to-byte
                    state.registers[a] = Value::MakeInt((int32_t)(int8_t)(state.registers[b].i & 0xFF));
                } else if (opcode == 0x8E) { // int-to-char
                    state.registers[a] = Value::MakeInt(state.registers[b].i & 0xFFFF);
                } else if (opcode == 0x8F) { // int-to-short
                    state.registers[a] = Value::MakeInt((int32_t)(int16_t)(state.registers[b].i & 0xFFFF));
                }
                
                Logger::d("Interpreter", "[0x7B..8F] unary op=" + std::to_string(opcode) + " v" + std::to_string(a) + " = op(v" + std::to_string(b) + ")");
                state.pc += 1;
                break;
            }
            case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96:
            case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:
            case 0x9E: case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4:
            case 0xA5: case 0xA6: case 0xA7: case 0xA8: case 0xA9: case 0xAA: case 0xAB:
            case 0xAC: case 0xAD: case 0xAE: case 0xAF: { // binop (23x)
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                uint8_t cc = safe8(bytecode, state.pc + 2);
                
                int valB = (state.registers[bb].type == ValueType::INT) ? state.registers[bb].i : 0;
                int valC = (state.registers[cc].type == ValueType::INT) ? state.registers[cc].i : 0;
                
                if (opcode <= 0xA5) { // int and long ops
                    int result = 0;
                    switch (opcode) {
                        case 0x90: case 0x9B: result = valB + valC; break;
                        case 0x91: case 0x9C: result = valB - valC; break;
                        case 0x92: case 0x9D: result = valB * valC; break;
                        case 0x93: case 0x9E: result = (valC != 0) ? (valB / valC) : 0; break;
                        case 0x94: case 0x9F: result = (valC != 0) ? (valB % valC) : 0; break;
                        case 0x95: case 0xA0: result = valB & valC; break;
                        case 0x96: case 0xA1: result = valB | valC; break;
                        case 0x97: case 0xA2: result = valB ^ valC; break;
                        case 0x98: result = valB << (valC & 0x1F); break;
                        case 0x99: result = valB >> (valC & 0x1F); break;
                        case 0x9A: result = (uint32_t)valB >> (valC & 0x1F); break;
                        case 0xA3: result = valB << (valC & 0x3F); break;
                        case 0xA4: result = valB >> (valC & 0x3F); break;
                        case 0xA5: result = (uint32_t)valB >> (valC & 0x3F); break;
                    }
                    state.registers[aa] = Value::MakeInt(result);
                } else { // float and double ops
                    float fvalB = (state.registers[bb].type == ValueType::FLOAT) ? state.registers[bb].f : (float)valB;
                    float fvalC = (state.registers[cc].type == ValueType::FLOAT) ? state.registers[cc].f : (float)valC;
                    float fresult = 0.0f;
                    switch (opcode) {
                        case 0xA6: case 0xAB: fresult = fvalB + fvalC; break;
                        case 0xA7: case 0xAC: fresult = fvalB - fvalC; break;
                        case 0xA8: case 0xAD: fresult = fvalB * fvalC; break;
                        case 0xA9: case 0xAE: fresult = (fvalC != 0.0f) ? (fvalB / fvalC) : 0.0f; break;
                        case 0xAA: case 0xAF: fresult = std::fmod(fvalB, fvalC); break;
                    }
                    state.registers[aa] = Value::MakeFloat(fresult);
                }
                
                Logger::d("Interpreter", "[0x90..AF] binop " + std::to_string(opcode) + " v" + std::to_string(aa) + " = v" + std::to_string(bb) + " op v" + std::to_string(cc));
                state.pc += 3;
                break;
            }
            case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7:
            case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC6: case 0xC7:
            case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF: { // binop/2addr (12x)
                uint8_t ab = safe8(bytecode, state.pc);
                uint8_t a = ab & 0x0F;
                uint8_t b = (ab >> 4) & 0x0F;
                
                int valA = (state.registers[a].type == ValueType::INT) ? state.registers[a].i : 0;
                int valB = (state.registers[b].type == ValueType::INT) ? state.registers[b].i : 0;
                
                if (opcode <= 0xC5) { // int and long ops
                    int result = 0;
                    switch (opcode) {
                        case 0xB0: case 0xBB: result = valA + valB; break;
                        case 0xB1: case 0xBC: result = valA - valB; break;
                        case 0xB2: case 0xBD: result = valA * valB; break;
                        case 0xB3: case 0xBE: result = (valB != 0) ? (valA / valB) : 0; break;
                        case 0xB4: case 0xBF: result = (valB != 0) ? (valA % valB) : 0; break;
                        case 0xB5: case 0xC0: result = valA & valB; break;
                        case 0xB6: case 0xC1: result = valA | valB; break;
                        case 0xB7: case 0xC2: result = valA ^ valB; break;
                        case 0xB8: result = valA << (valB & 0x1F); break;
                        case 0xB9: result = valA >> (valB & 0x1F); break;
                        case 0xBA: result = (uint32_t)valA >> (valB & 0x1F); break;
                        case 0xC3: result = valA << (valB & 0x3F); break;
                        case 0xC4: result = valA >> (valB & 0x3F); break;
                        case 0xC5: result = (uint32_t)valA >> (valB & 0x3F); break;
                    }
                    state.registers[a] = Value::MakeInt(result);
                } else { // float and double ops
                    float fvalA = (state.registers[a].type == ValueType::FLOAT) ? state.registers[a].f : (float)valA;
                    float fvalB = (state.registers[b].type == ValueType::FLOAT) ? state.registers[b].f : (float)valB;
                    float fresult = 0.0f;
                    switch (opcode) {
                        case 0xC6: case 0xCB: fresult = fvalA + fvalB; break;
                        case 0xC7: case 0xCC: fresult = fvalA - fvalB; break;
                        case 0xC8: case 0xCD: fresult = fvalA * fvalB; break;
                        case 0xC9: case 0xCE: fresult = (fvalB != 0.0f) ? (fvalA / fvalB) : 0.0f; break;
                        case 0xCA: case 0xCF: fresult = std::fmod(fvalA, fvalB); break;
                    }
                    state.registers[a] = Value::MakeFloat(fresult);
                }
                
                Logger::d("Interpreter", "[0xB0..CF] binop/2addr " + std::to_string(opcode) + " v" + std::to_string(a) + " = v" + std::to_string(a) + " op v" + std::to_string(b));
                state.pc += 1;
                break;
            }
            case 0xD0: case 0xD1: case 0xD2: case 0xD3:
            case 0xD4: case 0xD5: case 0xD6: case 0xD7: { // binop/lit16
                uint8_t ab = safe8(bytecode, state.pc);
                uint8_t a = ab & 0x0F;
                uint8_t b = (ab >> 4) & 0x0F;
                int16_t cccc = safe16(bytecode, state.pc + 1);
                int valB = (state.registers[b].type == ValueType::INT) ? state.registers[b].i : 0;
                int result = 0;
                switch (opcode) {
                    case 0xD0: result = valB + cccc; break;
                    case 0xD1: result = cccc - valB; break; // rsub-int (reverse subtract)
                    case 0xD2: result = valB * cccc; break;
                    case 0xD3: result = (cccc != 0) ? (valB / cccc) : 0; break;
                    case 0xD4: result = (cccc != 0) ? (valB % cccc) : 0; break;
                    case 0xD5: result = valB & cccc; break;
                    case 0xD6: result = valB | cccc; break;
                    case 0xD7: result = valB ^ cccc; break;
                }
                state.registers[a] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0xD0..D7] binop/lit16 (opcode=" + std::to_string(opcode) + ") v" + std::to_string(a) + " = v" + std::to_string(b) + " op " + std::to_string(cccc) + " -> " + std::to_string(result));
                state.pc += 3;
                break;
            }
            case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC:
            case 0xDD: case 0xDE: case 0xDF: case 0xE0: case 0xE1:
            case 0xE2: { // binop/lit8
                uint8_t aa = safe8(bytecode, state.pc);
                uint8_t bb = safe8(bytecode, state.pc + 1);
                int8_t cc = (int8_t)safe8(bytecode, state.pc + 2);
                int valB = (state.registers[bb].type == ValueType::INT) ? state.registers[bb].i : 0;
                int result = 0;
                switch (opcode) {
                    case 0xD8: result = valB + cc; break;
                    case 0xD9: result = cc - valB; break; // rsub-int/lit8
                    case 0xDA: result = valB * cc; break;
                    case 0xDB: result = (cc != 0) ? (valB / cc) : 0; break;
                    case 0xDC: result = (cc != 0) ? (valB % cc) : 0; break;
                    case 0xDD: result = valB & cc; break;
                    case 0xDE: result = valB | cc; break;
                    case 0xDF: result = valB ^ cc; break;
                    case 0xE0: result = valB << (cc & 0x1F); break;
                    case 0xE1: result = valB >> (cc & 0x1F); break;
                    case 0xE2: result = (uint32_t)valB >> (cc & 0x1F); break;
                }
                state.registers[aa] = Value::MakeInt(result);
                Logger::d("Interpreter", "[0xD8..E2] binop/lit8 (opcode=" + std::to_string(opcode) + ") v" + std::to_string(aa) + " = v" + std::to_string(bb) + " op " + std::to_string((int)cc) + " -> " + std::to_string(result));
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
