/*
 * NOTE: This file (dalvik_opcodes.cpp) was used in the initial phase as a standalone reference.
 * The actual, currently used interpreter loop is located in:
 * C:\Users\namde\Documents\Windroid\src\interpreter\Interpreter.cpp
 */

#include <cstdint>
#include <cstring>
#include <iostream>
#include <cmath>

// Logger macro for debugging
// Set to 1 to enable, 0 to disable
#define ENABLE_VM_DEBUG 1
#if ENABLE_VM_DEBUG
    #define VM_LOG(...) do { std::cout << "[VM_DEBUG] " << __VA_ARGS__ << std::endl; } while(0)
#else
    #define VM_LOG(...) do {} while(0)
#endif

// Forward declarations for VM State
struct VMState {
    const uint8_t* bytecode;
    uint32_t pc;         // Offset in bytes, pointing to the byte IMMEDIATELY AFTER the opcode
    uint32_t* registers; // 32-bit registers (wide values use 2 adjacent registers)
    
    // Formalized hooks for Windroid native integration
    void* exception_register;          // Pointer to C++ Exception wrapper
    void* result_register_object;      // Pointer to returned C++ Object wrapper
    uint64_t result_register_primitive;// Primitive return value (up to 64-bit)
};

// Helper macros to fetch bytes given the current state.pc (which is opcode + 1)
#define READ_U8(offset)  (state.bytecode[state.pc + (offset)])
#define READ_U16(offset) (uint16_t)(state.bytecode[state.pc + (offset)] | \
                                   (state.bytecode[state.pc + (offset) + 1] << 8))
#define READ_S16(offset) (int16_t)READ_U16(offset)
#define READ_U32(offset) (uint32_t)(state.bytecode[state.pc + (offset)] | \
                                   (state.bytecode[state.pc + (offset) + 1] << 8) | \
                                   (state.bytecode[state.pc + (offset) + 2] << 16) | \
                                   (state.bytecode[state.pc + (offset) + 3] << 24))
#define READ_S32(offset) (int32_t)READ_U32(offset)

// Helper inline functions for 64-bit and float access
inline int64_t GetReg64(VMState& state, uint8_t reg) {
    return ((int64_t)state.registers[reg]) | ((int64_t)state.registers[reg + 1] << 32);
}
inline void SetReg64(VMState& state, uint8_t reg, int64_t val) {
    state.registers[reg] = (uint32_t)val;
    state.registers[reg + 1] = (uint32_t)(val >> 32);
}
inline float GetRegFloat(VMState& state, uint8_t reg) {
    float f; std::memcpy(&f, &state.registers[reg], sizeof(float)); return f;
}
inline void SetRegFloat(VMState& state, uint8_t reg, float val) {
    std::memcpy(&state.registers[reg], &val, sizeof(float));
}
inline double GetRegDouble(VMState& state, uint8_t reg) {
    double d; uint64_t v = GetReg64(state, reg); std::memcpy(&d, &v, sizeof(double)); return d;
}
inline void SetRegDouble(VMState& state, uint8_t reg, double val) {
    uint64_t v; std::memcpy(&v, &val, sizeof(double)); SetReg64(state, reg, v);
}

// =========================================================================
// Native Bridge Hooks (To be implemented in your C++ framework logic)
// =========================================================================

// String & Class Resolution
// Used for const-string and const-class opcodes to resolve DEX pool indices into C++ object wrappers
void* VM_ResolveString(VMState& state, uint32_t string_idx) { return nullptr; }
void* VM_ResolveClass(VMState& state, uint32_t type_idx) { return nullptr; }

// Monitor (Synchronization)
// Hook into Win32 CriticalSections, Mutexes, or std::mutex for monitor-enter/exit
void VM_MonitorEnter(VMState& state, void* obj) {}
void VM_MonitorExit(VMState& state, void* obj) {}

// Object Instantiation & Type Checks
// Map type_idx to your native C++ classes and allocate them
void* VM_CheckCast(VMState& state, void* obj, uint32_t type_idx) { return nullptr; } // Throws ClassCastException if fails
uint32_t VM_InstanceOf(VMState& state, void* obj, uint32_t type_idx) { return 0; } // Returns 1 if true, 0 if false
void* VM_NewInstance(VMState& state, uint32_t type_idx) { return nullptr; }
void* VM_NewArray(VMState& state, uint32_t type_idx, int32_t length) { return nullptr; }
int32_t VM_ArrayLength(VMState& state, void* array_obj) { return 0; }

// Method Invocation
// Bridge Dalvik invoke-* instructions to your native C++ functions (e.g. android::view::View::measure)
void VM_InvokeMethod(VMState& state, uint32_t method_idx, const uint32_t* args, uint32_t arg_count, int invoke_type) {}

// Field and Array Access (Needs your internal Object/Array layout)
// These map to iget/iput/sget/sput and aget/aput
uint64_t VM_GetFieldPrimitive(VMState& state, void* obj, uint32_t field_idx) { return 0; }
void* VM_GetFieldObject(VMState& state, void* obj, uint32_t field_idx) { return nullptr; }
void VM_SetFieldPrimitive(VMState& state, void* obj, uint32_t field_idx, uint64_t val) {}
void VM_SetFieldObject(VMState& state, void* obj, uint32_t field_idx, void* val) {}

// Array Access
uint64_t VM_ArrayGetPrimitive(VMState& state, void* array_obj, int32_t index) { return 0; }
void* VM_ArrayGetObject(VMState& state, void* array_obj, int32_t index) { return nullptr; }
void VM_ArrayPutPrimitive(VMState& state, void* array_obj, int32_t index, uint64_t val) {}
void VM_ArrayPutObject(VMState& state, void* array_obj, int32_t index, void* val) {}


void ExecuteInstruction(VMState& state, uint8_t opcode) {
    VM_LOG("PC: 0x" << std::hex << (state.pc - 1) << " | Opcode: 0x" << (int)opcode << std::dec);

    switch (opcode) {

        // ==========================================
        // 0x00..0x09: Move instructions
        // ==========================================
        case 0x00: { // nop (10x)
            state.pc += 1;
            break;
        }
        case 0x01:   // move vA, vB (12x)
        case 0x04:   // move-wide vA, vB (12x)
        case 0x07: { // move-object vA, vB (12x)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            
            state.registers[vA] = state.registers[vB];
            if (opcode == 0x04) { // wide
                state.registers[vA + 1] = state.registers[vB + 1];
            }
            state.pc += 1;
            break;
        }
        case 0x02:   // move/from16 vAA, vBBBB (22x)
        case 0x05:   // move-wide/from16 vAA, vBBBB (22x)
        case 0x08: { // move-object/from16 vAA, vBBBB (22x)
            uint8_t vAA = READ_U8(0);
            uint16_t vBBBB = READ_U16(1);
            
            state.registers[vAA] = state.registers[vBBBB];
            if (opcode == 0x05) { // wide
                state.registers[vAA + 1] = state.registers[vBBBB + 1];
            }
            state.pc += 3;
            break;
        }
        case 0x03:   // move/16 vAAAA, vBBBB (32x)
        case 0x06:   // move-wide/16 vAAAA, vBBBB (32x)
        case 0x09: { // move-object/16 vAAAA, vBBBB (32x)
            uint16_t vAAAA = READ_U16(1);
            uint16_t vBBBB = READ_U16(3);
            
            state.registers[vAAAA] = state.registers[vBBBB];
            if (opcode == 0x06) { // wide
                state.registers[vAAAA + 1] = state.registers[vBBBB + 1];
            }
            state.pc += 5;
            break;
        }

        // ==========================================
        // 0x0A..0x0D: Move result instructions
        // ==========================================
        case 0x0A:   // move-result vAA (11x)
        case 0x0B: { // move-result-wide vAA (11x)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)state.result_register_primitive;
            if (opcode == 0x0B) state.registers[vAA + 1] = (uint32_t)(state.result_register_primitive >> 32);
            state.pc += 1;
            break;
        }
        case 0x0C: { // move-result-object vAA (11x)
            uint8_t vAA = READ_U8(0);
            // Treat object pointers as 32-bit or 64-bit depending on architecture
            state.registers[vAA] = (uint32_t)(uintptr_t)state.result_register_object;
            state.pc += 1;
            break;
        }
        case 0x0D: { // move-exception vAA (11x)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)(uintptr_t)state.exception_register;
            state.exception_register = nullptr; // Clear exception after moving
            state.pc += 1;
            break;
        }

        // ==========================================
        // 0x0E..0x11: Return instructions
        // ==========================================
        case 0x0E: { // return-void (10x)
            VM_LOG("return-void");
            state.pc += 1;
            break;
        }
        case 0x0F:   // return vAA (11x)
        case 0x10: { // return-wide vAA (11x)
            uint8_t vAA = READ_U8(0);
            state.result_register_primitive = state.registers[vAA];
            if (opcode == 0x10) state.result_register_primitive |= ((uint64_t)state.registers[vAA + 1] << 32);
            VM_LOG("return primitive: " << state.result_register_primitive);
            state.pc += 1;
            break;
        }
        case 0x11: { // return-object vAA (11x)
            uint8_t vAA = READ_U8(0);
            state.result_register_object = (void*)(uintptr_t)state.registers[vAA];
            VM_LOG("return object");
            state.pc += 1;
            break;
        }

        // ==========================================
        // 0x12..0x1C: Constant loading
        // ==========================================
        case 0x12: { // const/4 vA, #+B (11n)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            int32_t B = (int32_t)(int8_t)(args & 0xF0) >> 4; // Sign-extend 4-bit literal
            state.registers[vA] = (uint32_t)B;
            state.pc += 1;
            break;
        }
        case 0x13: { // const/16 vAA, #+BBBB (21s)
            uint8_t vAA = READ_U8(0);
            int32_t BBBB = READ_S16(1); // Sign-extend 16-bit literal
            state.registers[vAA] = (uint32_t)BBBB;
            state.pc += 3;
            break;
        }
        case 0x14: { // const vAA, #+BBBBBBBB (31i)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = READ_U32(1);
            state.pc += 5;
            break;
        }
        case 0x15: { // const/high16 vAA, #+BBBB0000 (21h)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = READ_U16(1) << 16;
            state.pc += 3;
            break;
        }
        case 0x16: { // const-wide/16 vAA, #+BBBB (21s)
            uint8_t vAA = READ_U8(0);
            SetReg64(state, vAA, READ_S16(1));
            state.pc += 3;
            break;
        }
        case 0x17: { // const-wide/32 vAA, #+BBBBBBBB (31i)
            uint8_t vAA = READ_U8(0);
            SetReg64(state, vAA, READ_S32(1));
            state.pc += 5;
            break;
        }
        case 0x18: { // const-wide vAA, #+BBBBBBBBBBBBBBBB (51l)
            uint8_t vAA = READ_U8(0);
            uint32_t lo = READ_U32(1);
            uint32_t hi = READ_U32(5);
            SetReg64(state, vAA, ((uint64_t)hi << 32) | lo);
            state.pc += 9;
            break;
        }
        case 0x19: { // const-wide/high16 vAA, #+BBBB000000000000 (21h)
            uint8_t vAA = READ_U8(0);
            SetReg64(state, vAA, (uint64_t)READ_U16(1) << 48);
            state.pc += 3;
            break;
        }
        case 0x1A: { // const-string vAA, string@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)(uintptr_t)VM_ResolveString(state, READ_U16(1));
            state.pc += 3;
            break;
        }
        case 0x1B: { // const-string/jumbo vAA, string@BBBBBBBB (31c)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)(uintptr_t)VM_ResolveString(state, READ_U32(1));
            state.pc += 5;
            break;
        }
        case 0x1C: { // const-class vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)(uintptr_t)VM_ResolveClass(state, READ_U16(1));
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x1D..0x20: Monitors and Instance checks
        // ==========================================
        case 0x1D: { // monitor-enter vAA (11x)
            VM_MonitorEnter(state, (void*)(uintptr_t)state.registers[READ_U8(0)]);
            state.pc += 1;
            break;
        }
        case 0x1E: { // monitor-exit vAA (11x)
            VM_MonitorExit(state, (void*)(uintptr_t)state.registers[READ_U8(0)]);
            state.pc += 1;
            break;
        }
        case 0x1F: { // check-cast vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            VM_CheckCast(state, (void*)(uintptr_t)state.registers[vAA], READ_U16(1));
            state.pc += 3;
            break;
        }
        case 0x20: { // instance-of vA, vB, type@CCCC (22c)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            state.registers[vA] = VM_InstanceOf(state, (void*)(uintptr_t)state.registers[vB], READ_U16(1));
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x21..0x26: Arrays and Allocation
        // ==========================================
        case 0x21: { // array-length vA, vB (12x)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            state.registers[vA] = VM_ArrayLength(state, (void*)(uintptr_t)state.registers[vB]);
            state.pc += 1;
            break;
        }
        case 0x22: { // new-instance vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            state.registers[vAA] = (uint32_t)(uintptr_t)VM_NewInstance(state, READ_U16(1));
            state.pc += 3;
            break;
        }
        case 0x23: { // new-array vA, vB, type@CCCC (22c)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            state.registers[vA] = (uint32_t)(uintptr_t)VM_NewArray(state, READ_U16(1), state.registers[vB]);
            state.pc += 3;
            break;
        }
        // ... (Skipping 0x24 filled-new-array and 0x25 filled-new-array/range for brevity as they require a lot of args decoding)
        
        case 0x26: { // fill-array-data vAA, +BBBBBBBB (31t)
            // Reads array data payload
            uint8_t vAA = READ_U8(0);
            int32_t offset = READ_S32(1);
            // const uint8_t* payload = state.bytecode + (state.pc - 1 + offset * 2);
            // Hook to VM_FillArrayData(...)
            state.pc += 5;
            break;
        }

        // ==========================================
        // 0x27..0x2A: Exceptions and jumps
        // ==========================================
        case 0x27: { // throw vAA (11x)
            uint8_t vAA = READ_U8(0);
            state.exception_register = (void*)(uintptr_t)state.registers[vAA];
            
            if (state.exception_register) {
                VM_LOG("throw exception");
            } else {
                VM_LOG("throw NullPointerException");
                // state.exception_register = createNullPointerException(); // Helper needed
            }
            // In a full interpreter loop, you would unwind to the caller here
            // e.g., isRunning = false; return;
            state.pc += 1;
            break;
        }
        case 0x28: { // goto +AA (10t)
            int8_t offset = (int8_t)READ_U8(0);
            state.pc += (offset * 2) - 1;
            break;
        }
        case 0x29: { // goto/16 +AAAA (20t)
            int16_t offset = READ_S16(1);
            state.pc += (offset * 2) - 1;
            break;
        }
        case 0x2A: { // goto/32 +AAAAAAAA (30t)
            int32_t offset = READ_S32(1);
            state.pc += (offset * 2) - 1;
            break;
        }

        // ==========================================
        // 0x2B..0x2C: Switch statements
        // ==========================================
        case 0x2B: { // packed-switch vAA, +BBBBBBBB (31t)
            uint8_t vAA = READ_U8(0);
            int32_t switchValue = (int32_t)state.registers[vAA];
            int32_t offset = READ_S32(1);
            
            // Payload is located at (opcode address + offset * 2)
            uint32_t payloadAddr = (state.pc - 1) + (offset * 2);
            
            // Format: ident(0x0100), size(uint16), first_key(int32), targets(int32[])
            // Reading unaligned bytes safely
            uint16_t ident = (uint16_t)(state.bytecode[payloadAddr] | (state.bytecode[payloadAddr + 1] << 8));
            if (ident == 0x0100) {
                int32_t targetsSize = (int32_t)(state.bytecode[payloadAddr + 2] | (state.bytecode[payloadAddr + 3] << 8));
                int32_t firstKey = (int32_t)(state.bytecode[payloadAddr + 4] | 
                                            (state.bytecode[payloadAddr + 5] << 8) | 
                                            (state.bytecode[payloadAddr + 6] << 16) | 
                                            (state.bytecode[payloadAddr + 7] << 24));
                
                uint32_t targetsOffset = payloadAddr + 8;
                int32_t index = switchValue - firstKey;
                
                int32_t targetOffset = 0;
                if (index >= 0 && index < targetsSize) {
                    uint32_t targetAddr = targetsOffset + (index * 4);
                    targetOffset = (int32_t)(state.bytecode[targetAddr] | 
                                            (state.bytecode[targetAddr + 1] << 8) | 
                                            (state.bytecode[targetAddr + 2] << 16) | 
                                            (state.bytecode[targetAddr + 3] << 24));
                    VM_LOG("packed-switch v" << (int)vAA << "=" << switchValue << " -> branch taken (offset " << targetOffset << ")");
                    state.pc = (state.pc - 1) + (targetOffset * 2);
                    break;
                }
            }
            VM_LOG("packed-switch v" << (int)vAA << "=" << switchValue << " -> default");
            state.pc += 5; // Default behavior
            break;
        }
        case 0x2C: { // sparse-switch vAA, +BBBBBBBB (31t)
            uint8_t vAA = READ_U8(0);
            int32_t switchValue = (int32_t)state.registers[vAA];
            int32_t offset = READ_S32(1);
            
            uint32_t payloadAddr = (state.pc - 1) + (offset * 2);
            
            uint16_t ident = (uint16_t)(state.bytecode[payloadAddr] | (state.bytecode[payloadAddr + 1] << 8));
            if (ident == 0x0200) {
                int32_t targetsSize = (int32_t)(state.bytecode[payloadAddr + 2] | (state.bytecode[payloadAddr + 3] << 8));
                
                uint32_t keysOffset = payloadAddr + 4;
                uint32_t targetsOffset = keysOffset + (targetsSize * 4);
                
                for (int i = 0; i < targetsSize; ++i) {
                    uint32_t keyAddr = keysOffset + (i * 4);
                    int32_t key = (int32_t)(state.bytecode[keyAddr] | 
                                            (state.bytecode[keyAddr + 1] << 8) | 
                                            (state.bytecode[keyAddr + 2] << 16) | 
                                            (state.bytecode[keyAddr + 3] << 24));
                                            
                    if (key == switchValue) {
                        uint32_t targetAddr = targetsOffset + (i * 4);
                        int32_t targetOffset = (int32_t)(state.bytecode[targetAddr] | 
                                                        (state.bytecode[targetAddr + 1] << 8) | 
                                                        (state.bytecode[targetAddr + 2] << 16) | 
                                                        (state.bytecode[targetAddr + 3] << 24));
                        VM_LOG("sparse-switch v" << (int)vAA << "=" << switchValue << " -> branch taken (offset " << targetOffset << ")");
                        state.pc = (state.pc - 1) + (targetOffset * 2);
                        goto sparse_branch_taken;
                    }
                }
            }
            VM_LOG("sparse-switch v" << (int)vAA << "=" << switchValue << " -> default");
            state.pc += 5; // Default
sparse_branch_taken:
            break;
        }

        // ==========================================
        // 0x2D..0x31: Comparisons
        // ==========================================
        case 0x2D: { // cmp-long vAA, vBB, vCC (23x)
            int64_t val1 = GetReg64(state, READ_U8(1));
            int64_t val2 = GetReg64(state, READ_U8(2));
            state.registers[READ_U8(0)] = (val1 == val2) ? 0 : ((val1 > val2) ? 1 : -1);
            state.pc += 3; break;
        }
        case 0x2E:   // cmpg-float vAA, vBB, vCC (23x)
        case 0x2F: { // cmpl-float vAA, vBB, vCC (23x)
            float val1 = GetRegFloat(state, READ_U8(1));
            float val2 = GetRegFloat(state, READ_U8(2));
            if (val1 > val2) state.registers[READ_U8(0)] = 1;
            else if (val1 == val2) state.registers[READ_U8(0)] = 0;
            else if (val1 < val2) state.registers[READ_U8(0)] = -1;
            else state.registers[READ_U8(0)] = (opcode == 0x2E) ? 1 : -1; // NaN handling
            state.pc += 3; break;
        }
        case 0x30:   // cmpg-double vAA, vBB, vCC (23x)
        case 0x31: { // cmpl-double vAA, vBB, vCC (23x)
            double val1 = GetRegDouble(state, READ_U8(1));
            double val2 = GetRegDouble(state, READ_U8(2));
            if (val1 > val2) state.registers[READ_U8(0)] = 1;
            else if (val1 == val2) state.registers[READ_U8(0)] = 0;
            else if (val1 < val2) state.registers[READ_U8(0)] = -1;
            else state.registers[READ_U8(0)] = (opcode == 0x30) ? 1 : -1;
            state.pc += 3; break;
        }

        // ==========================================
        // 0x32..0x3D: If tests
        // ==========================================
        case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37: { // (22t)
            uint8_t args = READ_U8(0);
            int32_t val1 = (int32_t)state.registers[args & 0x0F];
            int32_t val2 = (int32_t)state.registers[(args >> 4) & 0x0F];
            bool cond = false;
            if (opcode == 0x32) cond = (val1 == val2); // if-eq
            else if (opcode == 0x33) cond = (val1 != val2); // if-ne
            else if (opcode == 0x34) cond = (val1 < val2);  // if-lt
            else if (opcode == 0x35) cond = (val1 >= val2); // if-ge
            else if (opcode == 0x36) cond = (val1 > val2);  // if-gt
            else if (opcode == 0x37) cond = (val1 <= val2); // if-le

            if (cond) state.pc += (READ_S16(1) * 2) - 1;
            else state.pc += 3;
            break;
        }
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: { // (21t)
            int32_t val = (int32_t)state.registers[READ_U8(0)];
            bool cond = false;
            if (opcode == 0x38) cond = (val == 0); // if-eqz
            else if (opcode == 0x39) cond = (val != 0); // if-nez
            else if (opcode == 0x3A) cond = (val < 0);  // if-ltz
            else if (opcode == 0x3B) cond = (val >= 0); // if-gez
            else if (opcode == 0x3C) cond = (val > 0);  // if-gtz
            else if (opcode == 0x3D) cond = (val <= 0); // if-lez

            if (cond) state.pc += (READ_S16(1) * 2) - 1;
            else state.pc += 3;
            break;
        }

        // ==========================================
        // 0x7B..0x8F: Unary operations
        // ==========================================
        case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: case 0x80: case 0x81:
        case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: case 0x88:
        case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: { // (12x)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;

            switch (opcode) {
                case 0x7B: state.registers[vA] = (uint32_t)(-(int32_t)state.registers[vB]); break; // neg-int
                case 0x7C: state.registers[vA] = ~state.registers[vB]; break; // not-int
                case 0x7D: SetReg64(state, vA, -GetReg64(state, vB)); break; // neg-long
                case 0x7E: SetReg64(state, vA, ~GetReg64(state, vB)); break; // not-long
                case 0x7F: SetRegFloat(state, vA, -GetRegFloat(state, vB)); break; // neg-float
                case 0x80: SetRegDouble(state, vA, -GetRegDouble(state, vB)); break; // neg-double
                case 0x81: SetReg64(state, vA, (int64_t)(int32_t)state.registers[vB]); break; // int-to-long
                case 0x82: SetRegFloat(state, vA, (float)(int32_t)state.registers[vB]); break; // int-to-float
                case 0x83: SetRegDouble(state, vA, (double)(int32_t)state.registers[vB]); break; // int-to-double
                case 0x84: state.registers[vA] = (uint32_t)GetReg64(state, vB); break; // long-to-int
                case 0x85: SetRegFloat(state, vA, (float)GetReg64(state, vB)); break; // long-to-float
                case 0x86: SetRegDouble(state, vA, (double)GetReg64(state, vB)); break; // long-to-double
                case 0x87: state.registers[vA] = (uint32_t)(int32_t)GetRegFloat(state, vB); break; // float-to-int
                case 0x88: SetReg64(state, vA, (int64_t)GetRegFloat(state, vB)); break; // float-to-long
                case 0x89: SetRegDouble(state, vA, (double)GetRegFloat(state, vB)); break; // float-to-double
                case 0x8A: state.registers[vA] = (uint32_t)(int32_t)GetRegDouble(state, vB); break; // double-to-int
                case 0x8B: SetReg64(state, vA, (int64_t)GetRegDouble(state, vB)); break; // double-to-long
                case 0x8C: SetRegFloat(state, vA, (float)GetRegDouble(state, vB)); break; // double-to-float
                case 0x8D: state.registers[vA] = (uint32_t)(int8_t)(state.registers[vB] & 0xFF); break; // int-to-byte
                case 0x8E: state.registers[vA] = (uint32_t)(uint16_t)(state.registers[vB] & 0xFFFF); break; // int-to-char
                case 0x8F: state.registers[vA] = (uint32_t)(int16_t)(state.registers[vB] & 0xFFFF); break; // int-to-short
            }
            state.pc += 1;
            break;
        }

        // ==========================================
        // 0x90..0xAF: Binary operations
        // ==========================================
        case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96:
        case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D:
        case 0x9E: case 0x9F: case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4:
        case 0xA5: case 0xA6: case 0xA7: case 0xA8: case 0xA9: case 0xAA: case 0xAB:
        case 0xAC: case 0xAD: case 0xAE: case 0xAF: { // (23x)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1);
            uint8_t vCC = READ_U8(2);

            switch (opcode) {
                case 0x90: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] + (int32_t)state.registers[vCC]); break; // add-int
                case 0x91: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] - (int32_t)state.registers[vCC]); break; // sub-int
                case 0x92: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] * (int32_t)state.registers[vCC]); break; // mul-int
                case 0x93: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] / (int32_t)state.registers[vCC]); break; // div-int
                case 0x94: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] % (int32_t)state.registers[vCC]); break; // rem-int
                case 0x95: state.registers[vAA] = state.registers[vBB] & state.registers[vCC]; break; // and-int
                case 0x96: state.registers[vAA] = state.registers[vBB] | state.registers[vCC]; break; // or-int
                case 0x97: state.registers[vAA] = state.registers[vBB] ^ state.registers[vCC]; break; // xor-int
                case 0x98: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] << (state.registers[vCC] & 0x1F)); break; // shl-int
                case 0x99: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] >> (state.registers[vCC] & 0x1F)); break; // shr-int
                case 0x9A: state.registers[vAA] = state.registers[vBB] >> (state.registers[vCC] & 0x1F); break; // ushr-int

                case 0x9B: SetReg64(state, vAA, GetReg64(state, vBB) + GetReg64(state, vCC)); break; // add-long
                case 0x9C: SetReg64(state, vAA, GetReg64(state, vBB) - GetReg64(state, vCC)); break; // sub-long
                case 0x9D: SetReg64(state, vAA, GetReg64(state, vBB) * GetReg64(state, vCC)); break; // mul-long
                case 0x9E: SetReg64(state, vAA, GetReg64(state, vBB) / GetReg64(state, vCC)); break; // div-long
                case 0x9F: SetReg64(state, vAA, GetReg64(state, vBB) % GetReg64(state, vCC)); break; // rem-long
                case 0xA0: SetReg64(state, vAA, GetReg64(state, vBB) & GetReg64(state, vCC)); break; // and-long
                case 0xA1: SetReg64(state, vAA, GetReg64(state, vBB) | GetReg64(state, vCC)); break; // or-long
                case 0xA2: SetReg64(state, vAA, GetReg64(state, vBB) ^ GetReg64(state, vCC)); break; // xor-long
                case 0xA3: SetReg64(state, vAA, GetReg64(state, vBB) << (state.registers[vCC] & 0x3F)); break; // shl-long
                case 0xA4: SetReg64(state, vAA, GetReg64(state, vBB) >> (state.registers[vCC] & 0x3F)); break; // shr-long
                case 0xA5: SetReg64(state, vAA, (uint64_t)GetReg64(state, vBB) >> (state.registers[vCC] & 0x3F)); break; // ushr-long

                case 0xA6: SetRegFloat(state, vAA, GetRegFloat(state, vBB) + GetRegFloat(state, vCC)); break; // add-float
                case 0xA7: SetRegFloat(state, vAA, GetRegFloat(state, vBB) - GetRegFloat(state, vCC)); break; // sub-float
                case 0xA8: SetRegFloat(state, vAA, GetRegFloat(state, vBB) * GetRegFloat(state, vCC)); break; // mul-float
                case 0xA9: SetRegFloat(state, vAA, GetRegFloat(state, vBB) / GetRegFloat(state, vCC)); break; // div-float
                case 0xAA: SetRegFloat(state, vAA, std::fmod(GetRegFloat(state, vBB), GetRegFloat(state, vCC))); break; // rem-float

                case 0xAB: SetRegDouble(state, vAA, GetRegDouble(state, vBB) + GetRegDouble(state, vCC)); break; // add-double
                case 0xAC: SetRegDouble(state, vAA, GetRegDouble(state, vBB) - GetRegDouble(state, vCC)); break; // sub-double
                case 0xAD: SetRegDouble(state, vAA, GetRegDouble(state, vBB) * GetRegDouble(state, vCC)); break; // mul-double
                case 0xAE: SetRegDouble(state, vAA, GetRegDouble(state, vBB) / GetRegDouble(state, vCC)); break; // div-double
                case 0xAF: SetRegDouble(state, vAA, std::fmod(GetRegDouble(state, vBB), GetRegDouble(state, vCC))); break; // rem-double
            }

            state.pc += 3;
            break;
        }

        // ==========================================
        // 0xB0..0xCF: Binary operations /2addr
        // ==========================================
        case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6:
        case 0xB7: case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD:
        case 0xBE: case 0xBF: case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4:
        case 0xC5: case 0xC6: case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xCC: case 0xCD: case 0xCE: case 0xCF: { // (12x)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;

            switch (opcode) {
                case 0xB0: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] + (int32_t)state.registers[vB]); break; // add-int/2addr
                case 0xB1: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] - (int32_t)state.registers[vB]); break; // sub-int/2addr
                case 0xB2: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] * (int32_t)state.registers[vB]); break; // mul-int/2addr
                case 0xB3: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] / (int32_t)state.registers[vB]); break; // div-int/2addr
                case 0xB4: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] % (int32_t)state.registers[vB]); break; // rem-int/2addr
                case 0xB5: state.registers[vA] &= state.registers[vB]; break; // and-int/2addr
                case 0xB6: state.registers[vA] |= state.registers[vB]; break; // or-int/2addr
                case 0xB7: state.registers[vA] ^= state.registers[vB]; break; // xor-int/2addr
                case 0xB8: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] << (state.registers[vB] & 0x1F)); break; // shl-int/2addr
                case 0xB9: state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] >> (state.registers[vB] & 0x1F)); break; // shr-int/2addr
                case 0xBA: state.registers[vA] >>= (state.registers[vB] & 0x1F); break; // ushr-int/2addr

                case 0xBB: SetReg64(state, vA, GetReg64(state, vA) + GetReg64(state, vB)); break; // add-long/2addr
                case 0xBC: SetReg64(state, vA, GetReg64(state, vA) - GetReg64(state, vB)); break; // sub-long/2addr
                case 0xBD: SetReg64(state, vA, GetReg64(state, vA) * GetReg64(state, vB)); break; // mul-long/2addr
                case 0xBE: SetReg64(state, vA, GetReg64(state, vA) / GetReg64(state, vB)); break; // div-long/2addr
                case 0xBF: SetReg64(state, vA, GetReg64(state, vA) % GetReg64(state, vB)); break; // rem-long/2addr
                case 0xC0: SetReg64(state, vA, GetReg64(state, vA) & GetReg64(state, vB)); break; // and-long/2addr
                case 0xC1: SetReg64(state, vA, GetReg64(state, vA) | GetReg64(state, vB)); break; // or-long/2addr
                case 0xC2: SetReg64(state, vA, GetReg64(state, vA) ^ GetReg64(state, vB)); break; // xor-long/2addr
                case 0xC3: SetReg64(state, vA, GetReg64(state, vA) << (state.registers[vB] & 0x3F)); break; // shl-long/2addr
                case 0xC4: SetReg64(state, vA, GetReg64(state, vA) >> (state.registers[vB] & 0x3F)); break; // shr-long/2addr
                case 0xC5: SetReg64(state, vA, (uint64_t)GetReg64(state, vA) >> (state.registers[vB] & 0x3F)); break; // ushr-long/2addr

                case 0xC6: SetRegFloat(state, vA, GetRegFloat(state, vA) + GetRegFloat(state, vB)); break; // add-float/2addr
                case 0xC7: SetRegFloat(state, vA, GetRegFloat(state, vA) - GetRegFloat(state, vB)); break; // sub-float/2addr
                case 0xC8: SetRegFloat(state, vA, GetRegFloat(state, vA) * GetRegFloat(state, vB)); break; // mul-float/2addr
                case 0xC9: SetRegFloat(state, vA, GetRegFloat(state, vA) / GetRegFloat(state, vB)); break; // div-float/2addr
                case 0xCA: SetRegFloat(state, vA, std::fmod(GetRegFloat(state, vA), GetRegFloat(state, vB))); break; // rem-float/2addr

                case 0xCB: SetRegDouble(state, vA, GetRegDouble(state, vA) + GetRegDouble(state, vB)); break; // add-double/2addr
                case 0xCC: SetRegDouble(state, vA, GetRegDouble(state, vA) - GetRegDouble(state, vB)); break; // sub-double/2addr
                case 0xCD: SetRegDouble(state, vA, GetRegDouble(state, vA) * GetRegDouble(state, vB)); break; // mul-double/2addr
                case 0xCE: SetRegDouble(state, vA, GetRegDouble(state, vA) / GetRegDouble(state, vB)); break; // div-double/2addr
                case 0xCF: SetRegDouble(state, vA, std::fmod(GetRegDouble(state, vA), GetRegDouble(state, vB))); break; // rem-double/2addr
            }

            state.pc += 1;
            break;
        }

        // ==========================================
        // 0xD0..0xD7: Binary operations /lit16
        // ==========================================
        case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD7: { // (22s)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            int16_t lit = READ_S16(1);

            switch (opcode) {
                case 0xD0: state.registers[vA] = (uint32_t)((int32_t)state.registers[vB] + lit); break; // add-int/lit16
                case 0xD1: state.registers[vA] = (uint32_t)(lit - (int32_t)state.registers[vB]); break; // rsub-int (reverse subtract)
                case 0xD2: state.registers[vA] = (uint32_t)((int32_t)state.registers[vB] * lit); break; // mul-int/lit16
                case 0xD3: state.registers[vA] = (uint32_t)((int32_t)state.registers[vB] / lit); break; // div-int/lit16
                case 0xD4: state.registers[vA] = (uint32_t)((int32_t)state.registers[vB] % lit); break; // rem-int/lit16
                case 0xD5: state.registers[vA] = state.registers[vB] & (uint32_t)lit; break; // and-int/lit16
                case 0xD6: state.registers[vA] = state.registers[vB] | (uint32_t)lit; break; // or-int/lit16
                case 0xD7: state.registers[vA] = state.registers[vB] ^ (uint32_t)lit; break; // xor-int/lit16
            }
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0xD8..0xE2: Binary operations /lit8
        // ==========================================
        case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE:
        case 0xDF: case 0xE0: case 0xE1: case 0xE2: { // (22b)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1);
            int32_t lit = (int32_t)(int8_t)READ_U8(2); // sign-extend

            switch (opcode) {
                case 0xD8: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] + lit); break; // add-int/lit8
                case 0xD9: state.registers[vAA] = (uint32_t)(lit - (int32_t)state.registers[vBB]); break; // rsub-int/lit8
                case 0xDA: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] * lit); break; // mul-int/lit8
                case 0xDB: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] / lit); break; // div-int/lit8
                case 0xDC: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] % lit); break; // rem-int/lit8
                case 0xDD: state.registers[vAA] = state.registers[vBB] & (uint32_t)lit; break; // and-int/lit8
                case 0xDE: state.registers[vAA] = state.registers[vBB] | (uint32_t)lit; break; // or-int/lit8
                case 0xDF: state.registers[vAA] = state.registers[vBB] ^ (uint32_t)lit; break; // xor-int/lit8
                case 0xE0: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] << (lit & 0x1F)); break; // shl-int/lit8
                case 0xE1: state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] >> (lit & 0x1F)); break; // shr-int/lit8
                case 0xE2: state.registers[vAA] = state.registers[vBB] >> (lit & 0x1F); break; // ushr-int/lit8
            }
            state.pc += 3;
            break;
        }

        default:
            std::cerr << "Unknown or unimplemented opcode: 0x" << std::hex << (int)opcode << std::endl;
            break;
    }
}
