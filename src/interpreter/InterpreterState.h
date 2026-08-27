/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once
#include <cstdint>

#include "Value.h"

struct InterpreterState {
    Value registers[256];
    uint32_t pc;
    Value methodReturnVal = Value::MakeNull();
    InterpreterObject* pendingException = nullptr;  // For throw propagation
    
    InterpreterState() {
        for (int i = 0; i < 256; i++) {
            registers[i] = Value::MakeNull();
        }
        pc = 0;
    }
};
