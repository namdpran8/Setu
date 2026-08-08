#pragma once
#include <cstdint>

#include "Value.h"

struct InterpreterState {
    Value registers[256];
    uint32_t pc;
    Value methodReturnVal = Value::MakeNull();
    
    InterpreterState() {
        for (int i = 0; i < 256; i++) {
            registers[i] = Value::MakeNull();
        }
        pc = 0;
    }
};
