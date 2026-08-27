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

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cassert>

#include "SolverVariable.h"
#include "ArrayRow.h"
#include "Cache.h"
#include "Metrics.h"

namespace setu::cassowary {

class ConstraintAnchor;
class ConstraintWidget;

class LinearSystem {
public:
    static constexpr bool FULL_DEBUG = false;
    static constexpr bool DEBUG = false;
    static constexpr bool DO_NOT_USE = false;
    static constexpr bool DEBUG_CONSTRAINTS = FULL_DEBUG;

    inline static bool USE_DEPENDENCY_ORDERING = false;
    inline static bool USE_BASIC_SYNONYMS = true;
    inline static bool SIMPLIFY_SYNONYMS = true;
    inline static bool USE_SYNONYMS = true;
    inline static bool SKIP_COLUMNS = true;
    inline static bool OPTIMIZED_ENGINE = false;

private:
    static constexpr int sPoolSize = 1000;
    
public:
    bool hasSimpleDefinition = false;
    int mVariablesID = 0;

private:
    std::unordered_map<std::string, SolverVariable*> mVariables;
    std::unique_ptr<Row> mGoal;

    int mTableSize = 32;
    int mMaxColumns = mTableSize;
    std::vector<ArrayRow*> mRows;

public:
    bool graphOptimizer = false;
    bool newgraphOptimizer = false;

private:
    std::vector<bool> mAlreadyTestedCandidates;

public:
    int mNumColumns = 1;
    int mNumRows = 0;

private:
    int mMaxRows = mTableSize;
    Cache mCache;

    std::vector<SolverVariable*> mPoolVariables;
    int mPoolVariablesCount = 0;

public:
    inline static Metrics* sMetrics = nullptr;

private:
    std::unique_ptr<Row> mTempGoal;

    class ValuesRow : public ArrayRow {
    public:
        explicit ValuesRow(Cache* cache);
    };

public:
    LinearSystem();
    ~LinearSystem() = default;

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:107
    void fillMetrics(Metrics* metrics);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:111
    static Metrics* getMetrics();

private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:144
    void increaseTableSize();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:167
    void releaseRows();

public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:190
    void reset();

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:231
    SolverVariable* createObjectVariable(ConstraintAnchor* anchor);

    inline static long ARRAY_ROW_CREATION = 0;
    inline static long OPTIMIZED_ARRAY_ROW_CREATION = 0;

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:265
    ArrayRow* createRow();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:289
    SolverVariable* createSlackVariable();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:305
    SolverVariable* createExtraVariable();

private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:328
    void addSingleError(ArrayRow* row, int sign, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:342
    SolverVariable* createVariable(const std::string& name, SolverVariable::Type type);

public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:363
    SolverVariable* createErrorVariable(int strength, const std::string& prefix);

private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:386
    SolverVariable* acquireSolverVariable(SolverVariable::Type type, const std::string& prefix);

public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:412
    Row* getGoal();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:416
    ArrayRow* getRow(int n);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:420
    float getValueFor(const std::string& name);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:429
    int getObjectVariableValue(ConstraintAnchor* anchor);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:450
    SolverVariable* getVariable(const std::string& name, SolverVariable::Type type);

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:468
    void minimize();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:515
    void minimizeGoal(Row* goal);

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:544
    void cleanupRows();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:575
    void addConstraint(ArrayRow* row);
private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:661
    void addRow(ArrayRow* row);
public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:709
    void removeRow(ArrayRow* row);

private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:739
    int optimize(Row* goal, bool b);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:893
    int enforceBFS(Row* goal);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1064
    void computeValues();

private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1075
    void displayRows();
public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1088
    void displayReadableRows();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1119
    void displayVariablesReadableRows();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1135
    int getMemoryUsed();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1146
    int getNumEquations();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1151
    int getNumVariables();
    
private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1158
    void displaySystemInformation();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1183
    void displaySolverVariables();
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1197
    std::string getDisplaySize(int n);
public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1209
    Cache* getCache();
private:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1213
    std::string getDisplayStrength(int strength);

public:
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1250
    void addGreaterThan(SolverVariable* a, SolverVariable* b, int margin, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1267
    void addGreaterBarrier(SolverVariable* a, SolverVariable* b, int margin, bool hasMatchConstraintWidgets);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1289
    void addLowerThan(SolverVariable* a, SolverVariable* b, int margin, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1306
    void addLowerBarrier(SolverVariable* a, SolverVariable* b, int margin, bool hasMatchConstraintWidgets);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1332
    void addCentering(SolverVariable* a, SolverVariable* b, int m1, float bias, SolverVariable* c, SolverVariable* d, int m2, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1349
    void addRatio(SolverVariable* a, SolverVariable* b, SolverVariable* c, SolverVariable* d, float ratio, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1368
    void addSynonym(SolverVariable* a, SolverVariable* b, int margin);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1396
    ArrayRow* addEquality(SolverVariable* a, SolverVariable* b, int margin, int strength);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1446
    void addEquality(SolverVariable* a, int value);

    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1497
    static ArrayRow* createRowDimensionPercent(LinearSystem* linearSystem, SolverVariable* variableA, SolverVariable* variableC, float percent);
    
    // C:\Users\namde\Documents\Windroid\thirdparty\constraintlayout-main\constraintlayout-main\constraintlayout\core\src\main\java\androidx\constraintlayout\core\LinearSystem.java:1515
    void addCenterPoint(ConstraintWidget* widget, ConstraintWidget* target, float angle, int radius);
};

}
