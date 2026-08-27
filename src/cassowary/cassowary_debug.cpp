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

#include "LinearSystem.h"
#include "SolverVariable.h"
#include <iostream>
#include <fstream>

using namespace setu::cassowary;

int main() {
    std::cout << "Starting Test 8" << std::endl;
    LinearSystem system;
    SolverVariable* x = system.getVariable("x", SolverVariable::Type::UNRESTRICTED);
    SolverVariable* y = system.getVariable("y", SolverVariable::Type::UNRESTRICTED);
    system.addEquality(y, 10);
    std::cout << "Added y=10" << std::endl;
    system.addEquality(x, 20);
    std::cout << "Added x=20" << std::endl;
    system.addGreaterThan(x, y, 5, SolverVariable::STRENGTH_HIGH);
    std::cout << "Added x >= y + 5" << std::endl;
    
    std::cout << "Minimizing..." << std::endl;
    system.minimize();
    std::cout << "Done minimizing." << std::endl;
    return 0;
}
