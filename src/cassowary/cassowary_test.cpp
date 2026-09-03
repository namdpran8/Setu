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

// cassowary_test.cpp — Step 3: Standalone unit test for Tier 1 Cassowary solver engine.
// Exercises LinearSystem, SolverVariable, ArrayRow without any Tier 2 (widget) dependencies.
//
// Tests focus on the constraint equation builder and solver core, using the same
// patterns that ConstraintWidget/ConstraintWidgetContainer would use.

#include "LinearSystem.h"
#include "SolverVariable.h"
#include "ArrayRow.h"

#include <iostream>
#include <cmath>
#include <cassert>
#include <stdlib.h>
#include <crtdbg.h>

using namespace setu::cassowary;

static bool approxEqual(float a, float b, float epsilon = 0.5f) {
    return std::fabs(a - b) < epsilon;
}

static void disableAssertDialog() {
#if defined(_MSC_VER)
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
#endif
}

/// Helper: create a properly registered UNRESTRICTED variable.
static SolverVariable* createVar(LinearSystem& system, const std::string& name) {
    return system.getVariable(name, SolverVariable::Type::UNRESTRICTED);
}

static int passed = 0;
static int total = 0;

static void check(bool condition, const std::string& desc) {
    total++;
    if (condition) {
        passed++;
        std::cout << "  [PASS] " << desc << "\n";
    } else {
        std::cout << "  [FAIL] " << desc << "\n";
    }
}

static void test1_simple_equality() {
    std::cout << "Test 1: Simple equality (x = 10)\n";
    LinearSystem system;
    SolverVariable* x = createVar(system, "x");
    system.addEquality(x, 10);
    system.minimize();
    check(approxEqual(x->computedValue, 10.0f),
          "x = " + std::to_string(x->computedValue) + " (expected 10)");
}

static void test2_two_variables_equality() {
    std::cout << "Test 2: Two variables (x = y + 5, y = 10)\n";
    LinearSystem system;
    SolverVariable* x = createVar(system, "x");
    SolverVariable* y = createVar(system, "y");
    system.addEquality(y, 10);
    system.addEquality(x, y, 5, SolverVariable::STRENGTH_FIXED);
    system.minimize();
    check(approxEqual(y->computedValue, 10.0f),
          "y = " + std::to_string(y->computedValue) + " (expected 10)");
    check(approxEqual(x->computedValue, 15.0f),
          "x = " + std::to_string(x->computedValue) + " (expected 15)");
}

static void test3_layout_box() {
    std::cout << "Test 3: Layout box (left=0, right=left+80)\n";
    LinearSystem system;
    SolverVariable* left = createVar(system, "left");
    SolverVariable* right = createVar(system, "right");
    system.addEquality(left, 0);
    system.addEquality(right, left, 80, SolverVariable::STRENGTH_FIXED);
    system.minimize();
    check(approxEqual(left->computedValue, 0.0f),
          "left = " + std::to_string(left->computedValue) + " (expected 0)");
    check(approxEqual(right->computedValue, 80.0f),
          "right = " + std::to_string(right->computedValue) + " (expected 80)");
}

static void test4_three_variable_chain() {
    std::cout << "Test 4: Three-variable chain (a=0, b=a+10, c=b+20)\n";
    LinearSystem system;
    SolverVariable* a = createVar(system, "a");
    SolverVariable* b = createVar(system, "b");
    SolverVariable* c = createVar(system, "c");
    system.addEquality(a, 0);
    system.addEquality(b, a, 10, SolverVariable::STRENGTH_FIXED);
    system.addEquality(c, b, 20, SolverVariable::STRENGTH_FIXED);
    system.minimize();
    check(approxEqual(a->computedValue, 0.0f),
          "a = " + std::to_string(a->computedValue) + " (expected 0)");
    check(approxEqual(b->computedValue, 10.0f),
          "b = " + std::to_string(b->computedValue) + " (expected 10)");
    check(approxEqual(c->computedValue, 30.0f),
          "c = " + std::to_string(c->computedValue) + " (expected 30)");
}

static void test5_centering() {
    std::cout << "Test 5: Centering (point centered between 0 and 100)\n";
    LinearSystem system;
    SolverVariable* left = createVar(system, "left");
    SolverVariable* right = createVar(system, "right");
    SolverVariable* cL = createVar(system, "cL");
    SolverVariable* cR = createVar(system, "cR");
    system.addEquality(left, 0);
    system.addEquality(right, 100);
    // cR = cL (zero-width point)
    system.addEquality(cR, cL, 0, SolverVariable::STRENGTH_FIXED);
    // Centering: (cL - left) = bias * (right - cR), bias = 0.5
    system.addCentering(cL, left, 0, 0.5f, right, cR, 0,
                        SolverVariable::STRENGTH_FIXED);
    system.minimize();
    check(approxEqual(left->computedValue, 0.0f),
          "left = " + std::to_string(left->computedValue) + " (expected 0)");
    check(approxEqual(right->computedValue, 100.0f),
          "right = " + std::to_string(right->computedValue) + " (expected 100)");
    check(approxEqual(cL->computedValue, 50.0f),
          "cL = " + std::to_string(cL->computedValue) + " (expected 50)");
    check(approxEqual(cR->computedValue, 50.0f),
          "cR = " + std::to_string(cR->computedValue) + " (expected 50)");
}

static void test6_reset_and_reuse() {
    std::cout << "Test 6: System reset and reuse\n";
    LinearSystem system;
    SolverVariable* x = createVar(system, "x");
    system.addEquality(x, 42);
    system.minimize();
    check(approxEqual(x->computedValue, 42.0f),
          "x = " + std::to_string(x->computedValue) + " (expected 42, before reset)");

    system.reset();
    SolverVariable* y = createVar(system, "y");
    system.addEquality(y, 99);
    system.minimize();
    check(approxEqual(y->computedValue, 99.0f),
          "y = " + std::to_string(y->computedValue) + " (expected 99, after reset)");
}

static void test7_ratio() {
    std::cout << "Test 7: Ratio constraint (b = a * 0.5, a = 100)\n";
    LinearSystem system;
    SolverVariable* a = createVar(system, "a");
    SolverVariable* b = createVar(system, "b");
    SolverVariable* zero = createVar(system, "zero");
    system.addEquality(zero, 0);
    system.addEquality(a, 100);
    std::cout << "[Test7] After a=100, a=" << a->computedValue << "\n";
    system.addRatio(b, zero, a, zero, 0.5f, SolverVariable::STRENGTH_FIXED);
    std::cout << "[Test7] After addRatio, b=" << b->computedValue << "\n";
    std::cout << "[Test7] a->isFinalValue=" << a->isFinalValue << " zero->isFinalValue=" << zero->isFinalValue << "\n";
    std::cout << "[Test7] System rows before minimize:\n";
    for (int i = 0; i < system.getNumEquations(); i++) {
        std::cout << "  Row " << i << ": " << system.getRow(i)->toReadableString() << "\n";
    }
    system.minimize();
    std::cout << "[Test7] After minimize, b=" << b->computedValue << "\n";
    check(approxEqual(a->computedValue, 100.0f),
          "a = " + std::to_string(a->computedValue) + " (expected 100)");
    check(approxEqual(b->computedValue, 50.0f),
          "b = " + std::to_string(b->computedValue) + " (expected 50)");
}

static void test8_inequality_with_error() {
    std::cout << "Test 8: Inequality (x >= y + 5) with error strength\n";
    LinearSystem system;
    SolverVariable* x = createVar(system, "x");
    SolverVariable* y = createVar(system, "y");
    system.addEquality(y, 10);
    // x = 20 (strong)
    system.addEquality(x, 20);
    std::cout << "[Test8] After x=20, x=" << x->computedValue << "\n";
    system.addGreaterThan(x, y, 5, SolverVariable::STRENGTH_HIGH);
    std::cout << "[Test8] After addGreaterThan, x=" << x->computedValue << "\n";
    std::cout << "[Test8] System rows before minimize:\n";
    for (int i = 0; i < system.getNumEquations(); i++) {
        std::cout << "  Row " << i << ": " << system.getRow(i)->toReadableString() << "\n";
    }
    system.minimize();
    std::cout << "[Test8] After minimize, x=" << x->computedValue << "\n";
    check(approxEqual(y->computedValue, 10.0f),
          "y = " + std::to_string(y->computedValue) + " (expected 10)");
    check(x->computedValue >= 14.5f,
          "x = " + std::to_string(x->computedValue) + " (expected >= 15)");
}

static void test9_synonym() {
    std::cout << "Test 9: Synonym (a = b, b = 42)\n";
    LinearSystem system;
    SolverVariable* a = createVar(system, "a");
    SolverVariable* b = createVar(system, "b");
    system.addEquality(b, 42);
    system.addSynonym(a, b, 0);
    system.minimize();
    check(approxEqual(b->computedValue, 42.0f),
          "b = " + std::to_string(b->computedValue) + " (expected 42)");
    check(a->mIsSynonym && a->mSynonym == b->id,
          "a is synonym of b (expected true)");
}

int cassowary_test_main() {
    disableAssertDialog();
    std::cout << "=== Cassowary Tier 1 Solver Tests ===\n\n";

    test1_simple_equality();
    test2_two_variables_equality();
    test3_layout_box();
    test4_three_variable_chain();
    test5_centering();
    test6_reset_and_reuse();
    test7_ratio();
    test8_inequality_with_error();
    test9_synonym();

    std::cout << "\n=== Results: " << passed << "/" << total << " checks passed ===\n";
    return (passed == total) ? 0 : 1;
}
