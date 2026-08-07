#pragma once
#include <cstdint>

#include "Value.h"

struct InterpreterState {
    uint32_t registers[256];
    uint32_t pc;
    Value methodReturnVal = Value::MakeNull();
};
