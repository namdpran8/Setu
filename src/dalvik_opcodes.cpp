#include <cstdint>
#include <cstring>
#include <iostream>

// Forward declarations for VM State
struct VMState {
    const uint8_t* bytecode;
    uint32_t pc;         // Offset in bytes, currently pointing to the byte IMMEDIATELY AFTER the opcode
    uint32_t* registers; // 32-bit registers (wide values use 2 adjacent registers)
    
    // TODO: Define these for your specific VM implementation
    // Object* exception_register;
    // Object* result_register_object;
    // uint64_t result_register_primitive;
};

// Helper macros to fetch bytes given the current state.pc (which is opcode + 1)
// Note: Dalvik bytecode is little-endian.
#define READ_U8(offset)  (state.bytecode[state.pc + (offset)])
#define READ_U16(offset) (uint16_t)(state.bytecode[state.pc + (offset)] | \
                                   (state.bytecode[state.pc + (offset) + 1] << 8))
#define READ_S16(offset) (int16_t)READ_U16(offset)
#define READ_U32(offset) (uint32_t)(state.bytecode[state.pc + (offset)] | \
                                   (state.bytecode[state.pc + (offset) + 1] << 8) | \
                                   (state.bytecode[state.pc + (offset) + 2] << 16) | \
                                   (state.bytecode[state.pc + (offset) + 3] << 24))
#define READ_S32(offset) (int32_t)READ_U32(offset)

void ExecuteInstruction(VMState& state, uint8_t opcode) {
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
            // READ_U8(0) is padding
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
        case 0x0B:   // move-result-wide vAA (11x)
        case 0x0C: { // move-result-object vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Read from VM's internal result register
            // state.registers[vAA] = (uint32_t)state.result_register_primitive;
            // if (opcode == 0x0B) state.registers[vAA + 1] = (uint32_t)(state.result_register_primitive >> 32);
            state.pc += 1;
            break;
        }
        case 0x0D: { // move-exception vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Read from VM's exception register and clear it
            // state.registers[vAA] = (uint32_t)state.exception_register;
            // state.exception_register = nullptr;
            state.pc += 1;
            break;
        }

        // ==========================================
        // 0x0E..0x11: Return instructions
        // ==========================================
        case 0x0E: { // return-void (10x)
            // TODO: Terminate current method frame, restore caller state
            state.pc += 1;
            break;
        }
        case 0x0F:   // return vAA (11x)
        case 0x10:   // return-wide vAA (11x)
        case 0x11: { // return-object vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Save state.registers[vAA] (and vAA+1 for wide) to caller's result register, terminate frame
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
            uint32_t BBBBBBBB = READ_U32(1);
            
            state.registers[vAA] = BBBBBBBB;
            state.pc += 5;
            break;
        }
        case 0x15: { // const/high16 vAA, #+BBBB0000 (21h)
            uint8_t vAA = READ_U8(0);
            uint32_t BBBB = READ_U16(1);
            
            state.registers[vAA] = BBBB << 16;
            state.pc += 3;
            break;
        }
        case 0x16: { // const-wide/16 vAA, #+BBBB (21s)
            uint8_t vAA = READ_U8(0);
            int64_t BBBB = READ_S16(1); // Sign-extend to 64-bit
            
            state.registers[vAA] = (uint32_t)BBBB;
            state.registers[vAA + 1] = (uint32_t)(BBBB >> 32);
            state.pc += 3;
            break;
        }
        case 0x17: { // const-wide/32 vAA, #+BBBBBBBB (31i)
            uint8_t vAA = READ_U8(0);
            int64_t BBBBBBBB = READ_S32(1); // Sign-extend to 64-bit
            
            state.registers[vAA] = (uint32_t)BBBBBBBB;
            state.registers[vAA + 1] = (uint32_t)(BBBBBBBB >> 32);
            state.pc += 5;
            break;
        }
        case 0x18: { // const-wide vAA, #+BBBBBBBBBBBBBBBB (51l)
            uint8_t vAA = READ_U8(0);
            uint32_t lo = READ_U32(1);
            uint32_t hi = READ_U32(5);
            
            state.registers[vAA] = lo;
            state.registers[vAA + 1] = hi;
            state.pc += 9;
            break;
        }
        case 0x19: { // const-wide/high16 vAA, #+BBBB000000000000 (21h)
            uint8_t vAA = READ_U8(0);
            uint64_t BBBB = READ_U16(1);
            uint64_t value = BBBB << 48;
            
            state.registers[vAA] = (uint32_t)value;
            state.registers[vAA + 1] = (uint32_t)(value >> 32);
            state.pc += 3;
            break;
        }
        case 0x1A: { // const-string vAA, string@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            uint16_t string_idx = READ_U16(1);
            // TODO: Resolve string@BBBB from DEX string pool and store reference in vAA
            state.pc += 3;
            break;
        }
        case 0x1B: { // const-string/jumbo vAA, string@BBBBBBBB (31c)
            uint8_t vAA = READ_U8(0);
            uint32_t string_idx = READ_U32(1);
            // TODO: Resolve string@BBBBBBBB from DEX string pool and store reference in vAA
            state.pc += 5;
            break;
        }
        case 0x1C: { // const-class vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            uint16_t type_idx = READ_U16(1);
            // TODO: Resolve type@BBBB to a Class object and store reference in vAA
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x1D..0x20: Monitors and Instance checks
        // ==========================================
        case 0x1D: { // monitor-enter vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Acquire monitor for object reference in vAA
            state.pc += 1;
            break;
        }
        case 0x1E: { // monitor-exit vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Release monitor for object reference in vAA
            state.pc += 1;
            break;
        }
        case 0x1F: { // check-cast vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            uint16_t type_idx = READ_U16(1);
            // TODO: Check if object in vAA can be cast to type@BBBB. Throw ClassCastException if not.
            state.pc += 3;
            break;
        }
        case 0x20: { // instance-of vA, vB, type@CCCC (22c)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            uint16_t type_idx = READ_U16(1);
            // TODO: Check if object in vB is instance of type@CCCC, store boolean result (1 or 0) in vA
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
            // TODO: Read array length from array object reference in vB, store in vA. Throw NullPointerException if vB is null.
            state.pc += 1;
            break;
        }
        case 0x22: { // new-instance vAA, type@BBBB (21c)
            uint8_t vAA = READ_U8(0);
            uint16_t type_idx = READ_U16(1);
            // TODO: Allocate uninitialized object of type@BBBB, store reference in vAA
            state.pc += 3;
            break;
        }
        case 0x23: { // new-array vA, vB, type@CCCC (22c)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            uint16_t type_idx = READ_U16(1);
            // TODO: Allocate array of type@CCCC with length from vB, store reference in vA
            state.pc += 3;
            break;
        }
        case 0x24: { // filled-new-array {vD, vE, vF, vG, vA}, type@CCCC (35c)
            uint8_t args = READ_U8(0);
            uint8_t arg_count = (args >> 4) & 0x0F; // A
            uint8_t vG = args & 0x0F;               // G
            uint16_t type_idx = READ_U16(1);        // BBBB
            uint8_t args2 = READ_U8(3);
            uint8_t vF = args2 & 0x0F;              // F
            uint8_t vE = (args2 >> 4) & 0x0F;       // E
            uint8_t args3 = READ_U8(4);
            uint8_t vD = args3 & 0x0F;              // D
            uint8_t vC = (args3 >> 4) & 0x0F;       // C
            // Arguments are {vC, vD, vE, vF, vG} depending on arg_count (0 to 5)
            // TODO: Allocate array of type@CCCC with length `arg_count`, fill with provided registers, store reference in result register
            state.pc += 5;
            break;
        }
        case 0x25: { // filled-new-array/range {vCCCC .. vNNNN}, type@BBBB (3rc)
            uint8_t arg_count = READ_U8(0);
            uint16_t type_idx = READ_U16(1);
            uint16_t vCCCC = READ_U16(3);
            // TODO: Allocate array of type@BBBB with length `arg_count`, fill with registers from vCCCC to vCCCC+arg_count-1, store in result register
            state.pc += 5;
            break;
        }
        case 0x26: { // fill-array-data vAA, +BBBBBBBB (31t)
            uint8_t vAA = READ_U8(0);
            int32_t offset = READ_S32(1); // Branch offset relative to opcode
            // TODO: Read array data payload at (state.pc - 1 + offset) and populate array object referenced by vAA
            state.pc += 5;
            break;
        }

        // ==========================================
        // 0x27..0x2A: Exceptions and jumps
        // ==========================================
        case 0x27: { // throw vAA (11x)
            uint8_t vAA = READ_U8(0);
            // TODO: Throw exception object referenced by vAA
            state.pc += 1;
            break;
        }
        case 0x28: { // goto +AA (10t)
            int8_t offset = (int8_t)READ_U8(0);
            state.pc += (offset * 2) - 1; // -1 because pc already advanced past opcode
            break;
        }
        case 0x29: { // goto/16 +AAAA (20t)
            // READ_U8(0) is padding
            int16_t offset = READ_S16(1);
            state.pc += (offset * 2) - 1;
            break;
        }
        case 0x2A: { // goto/32 +AAAAAAAA (30t)
            // READ_U8(0) is padding
            int32_t offset = READ_S32(1);
            state.pc += (offset * 2) - 1;
            break;
        }

        // ==========================================
        // 0x2B..0x2C: Switch statements
        // ==========================================
        case 0x2B: { // packed-switch vAA, +BBBBBBBB (31t)
            uint8_t vAA = READ_U8(0);
            int32_t offset = READ_S32(1);
            // TODO: Read packed-switch-payload at (state.pc - 1 + offset), compare with state.registers[vAA], update pc if match found
            state.pc += 5; // Default behavior if no branch taken
            break;
        }
        case 0x2C: { // sparse-switch vAA, +BBBBBBBB (31t)
            uint8_t vAA = READ_U8(0);
            int32_t offset = READ_S32(1);
            // TODO: Read sparse-switch-payload at (state.pc - 1 + offset), compare with state.registers[vAA], update pc if match found
            state.pc += 5;
            break;
        }

        // ==========================================
        // 0x2D..0x31: Comparisons
        // ==========================================
        case 0x2D: { // cmp-long vAA, vBB, vCC (23x)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1);
            uint8_t vCC = READ_U8(2);
            int64_t val1 = ((uint64_t)state.registers[vBB]) | ((uint64_t)state.registers[vBB + 1] << 32);
            int64_t val2 = ((uint64_t)state.registers[vCC]) | ((uint64_t)state.registers[vCC + 1] << 32);
            
            if (val1 == val2) state.registers[vAA] = 0;
            else if (val1 > val2) state.registers[vAA] = 1;
            else state.registers[vAA] = (uint32_t)-1;
            
            state.pc += 3;
            break;
        }
        case 0x2E:   // cmpg-float vAA, vBB, vCC (23x)
        case 0x2F: { // cmpl-float vAA, vBB, vCC (23x)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1);
            uint8_t vCC = READ_U8(2);
            float val1; std::memcpy(&val1, &state.registers[vBB], sizeof(float));
            float val2; std::memcpy(&val2, &state.registers[vCC], sizeof(float));
            
            // TODO: Handle NaN properly
            if (val1 > val2) state.registers[vAA] = 1;
            else if (val1 == val2) state.registers[vAA] = 0;
            else if (val1 < val2) state.registers[vAA] = (uint32_t)-1;
            else state.registers[vAA] = (opcode == 0x2E) ? 1 : (uint32_t)-1; // NaN handling
            
            state.pc += 3;
            break;
        }
        case 0x30:   // cmpg-double vAA, vBB, vCC (23x)
        case 0x31: { // cmpl-double vAA, vBB, vCC (23x)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1);
            uint8_t vCC = READ_U8(2);
            
            double val1, val2;
            uint64_t raw1 = ((uint64_t)state.registers[vBB]) | ((uint64_t)state.registers[vBB + 1] << 32);
            uint64_t raw2 = ((uint64_t)state.registers[vCC]) | ((uint64_t)state.registers[vCC + 1] << 32);
            std::memcpy(&val1, &raw1, sizeof(double));
            std::memcpy(&val2, &raw2, sizeof(double));
            
            // TODO: Handle NaN properly
            if (val1 > val2) state.registers[vAA] = 1;
            else if (val1 == val2) state.registers[vAA] = 0;
            else if (val1 < val2) state.registers[vAA] = (uint32_t)-1;
            else state.registers[vAA] = (opcode == 0x30) ? 1 : (uint32_t)-1; // NaN handling
            
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x32..0x3D: If tests
        // ==========================================
        case 0x32: // if-eq vA, vB, +CCCC (22t)
        case 0x33: // if-ne vA, vB, +CCCC (22t)
        case 0x34: // if-lt vA, vB, +CCCC (22t)
        case 0x35: // if-ge vA, vB, +CCCC (22t)
        case 0x36: // if-gt vA, vB, +CCCC (22t)
        case 0x37: { // if-le vA, vB, +CCCC (22t)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F;
            int16_t offset = READ_S16(1);
            
            int32_t val1 = (int32_t)state.registers[vA];
            int32_t val2 = (int32_t)state.registers[vB];
            
            bool condition = false;
            if (opcode == 0x32) condition = (val1 == val2);
            else if (opcode == 0x33) condition = (val1 != val2);
            else if (opcode == 0x34) condition = (val1 < val2);
            else if (opcode == 0x35) condition = (val1 >= val2);
            else if (opcode == 0x36) condition = (val1 > val2);
            else if (opcode == 0x37) condition = (val1 <= val2);

            if (condition) state.pc += (offset * 2) - 1;
            else state.pc += 3;
            break;
        }
        case 0x38: // if-eqz vAA, +BBBB (21t)
        case 0x39: // if-nez vAA, +BBBB (21t)
        case 0x3A: // if-ltz vAA, +BBBB (21t)
        case 0x3B: // if-gez vAA, +BBBB (21t)
        case 0x3C: // if-gtz vAA, +BBBB (21t)
        case 0x3D: { // if-lez vAA, +BBBB (21t)
            uint8_t vAA = READ_U8(0);
            int16_t offset = READ_S16(1);
            
            int32_t val = (int32_t)state.registers[vAA];
            
            bool condition = false;
            if (opcode == 0x38) condition = (val == 0);
            else if (opcode == 0x39) condition = (val != 0);
            else if (opcode == 0x3A) condition = (val < 0);
            else if (opcode == 0x3B) condition = (val >= 0);
            else if (opcode == 0x3C) condition = (val > 0);
            else if (opcode == 0x3D) condition = (val <= 0);

            if (condition) state.pc += (offset * 2) - 1;
            else state.pc += 3;
            break;
        }
        
        // ==========================================
        // 0x44..0x51: Array operations
        // ==========================================
        // aget, aget-wide, aget-object, aget-boolean, aget-byte, aget-char, aget-short (0x44-0x4A)
        // aput, aput-wide, aput-object, aput-boolean, aput-byte, aput-char, aput-short (0x4B-0x51)
        case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A:
        case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: case 0x50: case 0x51: { // (23x)
            uint8_t vAA = READ_U8(0);
            uint8_t vBB = READ_U8(1); // Array object reference
            uint8_t vCC = READ_U8(2); // Index
            
            // TODO: Implement array getters/setters based on opcode (read from/write to array object)
            // Throw NullPointerException or ArrayIndexOutOfBoundsException appropriately.
            
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x52..0x5F: Instance fields
        // ==========================================
        // iget... (0x52-0x58), iput... (0x59-0x5F)
        case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58:
        case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: { // (22c)
            uint8_t args = READ_U8(0);
            uint8_t vA = args & 0x0F;
            uint8_t vB = (args >> 4) & 0x0F; // Object reference
            uint16_t field_idx = READ_U16(1);
            
            // TODO: Read/write instance field resolved by field_idx on object in vB
            
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x60..0x6D: Static fields
        // ==========================================
        // sget... (0x60-0x66), sput... (0x67-0x6D)
        case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66:
        case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: { // (21c)
            uint8_t vAA = READ_U8(0);
            uint16_t field_idx = READ_U16(1);
            
            // TODO: Read/write static field resolved by field_idx to/from vAA
            
            state.pc += 3;
            break;
        }

        // ==========================================
        // 0x6E..0x78: Method invocations
        // ==========================================
        case 0x6E: // invoke-virtual
        case 0x6F: // invoke-super
        case 0x70: // invoke-direct
        case 0x71: // invoke-static
        case 0x72: { // invoke-interface (35c)
            uint8_t args = READ_U8(0);
            uint8_t arg_count = (args >> 4) & 0x0F; // A
            uint8_t vG = args & 0x0F;               // G
            uint16_t method_idx = READ_U16(1);      // BBBB
            uint8_t args2 = READ_U8(3);
            uint8_t vF = args2 & 0x0F;              // F
            uint8_t vE = (args2 >> 4) & 0x0F;       // E
            uint8_t args3 = READ_U8(4);
            uint8_t vD = args3 & 0x0F;              // D
            uint8_t vC = (args3 >> 4) & 0x0F;       // C
            
            // Arguments are {vC, vD, vE, vF, vG} depending on arg_count (0 to 5)
            // TODO: Resolve method, setup new frame with arguments, branch to method entry
            
            state.pc += 5;
            break;
        }
        case 0x74: // invoke-virtual/range
        case 0x75: // invoke-super/range
        case 0x76: // invoke-direct/range
        case 0x77: // invoke-static/range
        case 0x78: { // invoke-interface/range (3rc)
            uint8_t arg_count = READ_U8(0);
            uint16_t method_idx = READ_U16(1);
            uint16_t vCCCC = READ_U16(3);
            
            // TODO: Setup frame using registers vCCCC to vCCCC+arg_count-1, branch to method
            
            state.pc += 5;
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
            
            if (opcode == 0x7B) { // neg-int
                state.registers[vA] = (uint32_t)(-(int32_t)state.registers[vB]);
            } else if (opcode == 0x7C) { // not-int
                state.registers[vA] = ~state.registers[vB];
            }
            // TODO: Implement other unary ops (neg-long, not-long, neg-float, neg-double, conversions)
            
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

            if (opcode == 0x90) { // add-int
                state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] + (int32_t)state.registers[vCC]);
            } else if (opcode == 0x91) { // sub-int
                state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] - (int32_t)state.registers[vCC]);
            }
            // TODO: Implement remaining binary operations (mul, div, rem, and, or, xor, shl, shr, ushr for int/long/float/double)

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

            if (opcode == 0xB0) { // add-int/2addr
                state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] + (int32_t)state.registers[vB]);
            } else if (opcode == 0xB1) { // sub-int/2addr
                state.registers[vA] = (uint32_t)((int32_t)state.registers[vA] - (int32_t)state.registers[vB]);
            }
            // TODO: Implement remaining /2addr binary operations

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
            int16_t CCCC = READ_S16(1);

            if (opcode == 0xD0) { // add-int/lit16
                state.registers[vA] = (uint32_t)((int32_t)state.registers[vB] + CCCC);
            } else if (opcode == 0xD1) { // rsub-int (reverse subtract)
                state.registers[vA] = (uint32_t)(CCCC - (int32_t)state.registers[vB]);
            }
            // TODO: Implement mul, div, rem, and, or, xor

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
            int32_t CC = (int32_t)(int8_t)READ_U8(2); // sign-extend

            if (opcode == 0xD8) { // add-int/lit8
                state.registers[vAA] = (uint32_t)((int32_t)state.registers[vBB] + CC);
            } else if (opcode == 0xD9) { // rsub-int/lit8
                state.registers[vAA] = (uint32_t)(CC - (int32_t)state.registers[vBB]);
            }
            // TODO: Implement mul, div, rem, and, or, xor, shl, shr, ushr

            state.pc += 3;
            break;
        }

        default:
            std::cerr << "Unknown or unimplemented opcode: " << std::hex << (int)opcode << std::endl;
            // TODO: Trigger VM abort or throw IllegalInstruction exception
            break;
    }
}
