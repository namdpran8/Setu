// ArrayRow.h — Ported from androidx.constraintlayout.core.ArrayRow (ArrayRow.java)
// Original: ArrayRow.java:1-814
//
// Represents a row in the constraint system's tableau.
// Implements the Row interface (from LinearSystem.java:115-135).
// Contains the ArrayRowVariables interface definition.
//
// OWNERSHIP MODEL:
// - ArrayRow instances are allocated by LinearSystem and recycled through Cache pools.
// - ArrayRow.mVariable is a raw non-owning pointer to SolverVariable.
// - ArrayRow.variables is an owning pointer (unique_ptr) to the ArrayRowVariables
//   implementation (ArrayLinkedVariables or SolverVariableValues).
// - mVariablesToUpdate is a transient working list, cleared after each use.
//
// Copyright (C) 2015 The Android Open Source Project — Apache 2.0

#pragma once

#include "SolverVariable.h"

#include <memory>
#include <string>
#include <vector>

namespace setu::cassowary {

// Forward declarations
class LinearSystem;
class Cache;

// ─── ArrayRow.java:39-85 — ArrayRowVariables interface ───
// Ported as an abstract base class. Implemented by ArrayLinkedVariables
// and SolverVariableValues.

/// Ported from ArrayRow.java:39 — public interface ArrayRowVariables
class ArrayRowVariables {
public:
    virtual ~ArrayRowVariables() = default;

    /// ArrayRow.java:42
    virtual int getCurrentSize() = 0;

    /// ArrayRow.java:45
    virtual SolverVariable* getVariable(int index) = 0;

    /// ArrayRow.java:48
    virtual float getVariableValue(int index) = 0;

    /// ArrayRow.java:51
    virtual float get(SolverVariable* variable) = 0;

    /// ArrayRow.java:54
    virtual int indexOf(SolverVariable* variable) = 0;

    /// ArrayRow.java:57
    virtual void display() = 0;

    /// ArrayRow.java:60
    virtual void clear() = 0;

    /// ArrayRow.java:63
    virtual bool contains(SolverVariable* variable) = 0;

    /// ArrayRow.java:66
    virtual void put(SolverVariable* variable, float value) = 0;

    /// ArrayRow.java:69
    virtual int sizeInBytes() = 0;

    /// ArrayRow.java:72
    virtual void invert() = 0;

    /// ArrayRow.java:75
    virtual float remove(SolverVariable* v, bool removeFromDefinition) = 0;

    /// ArrayRow.java:78
    virtual void divideByAmount(float amount) = 0;

    /// ArrayRow.java:81
    virtual void add(SolverVariable* v, float value, bool removeFromDefinition) = 0;

    /// ArrayRow.java:84
    virtual float use(ArrayRow* definition, bool removeFromDefinition) = 0;
};

// ─── LinearSystem.java:115-135 — Row interface ───
// Ported as an abstract base class. Implemented by ArrayRow, GoalRow, PriorityGoalRow.

/// Ported from LinearSystem.java:115 — interface Row
class Row {
public:
    virtual ~Row() = default;

    virtual SolverVariable* getPivotCandidate(LinearSystem* system, std::vector<bool>& avoid) = 0;
    virtual void clear() = 0;
    virtual void initFromRow(Row* row) = 0;
    virtual void addError(SolverVariable* variable) = 0;
    virtual void updateFromSystem(LinearSystem* system) = 0;
    virtual SolverVariable* getKey() = 0;
    virtual bool isEmpty() = 0;
    virtual void updateFromRow(LinearSystem* system, ArrayRow* definition, bool b) = 0;
    virtual void updateFromFinalVariable(LinearSystem* system,
                                          SolverVariable* variable,
                                          bool removeFromDefinition) = 0;
};

/// Ported from ArrayRow.java:27 — public class ArrayRow implements LinearSystem.Row
class ArrayRow : public Row {
public:
    // ─── ArrayRow.java:28-37 — Fields ───
    static constexpr bool DEBUG = false;
    static constexpr bool FULL_NEW_CHECK = false;

    SolverVariable* mVariable = nullptr;
    float mConstantValue = 0;
    bool mUsed = false;
    bool mIsSimpleDefinition = false;

    /// ArrayRow.java:35 — ArrayList<SolverVariable> mVariablesToUpdate
    /// Transient working list used in updateFromSystem(). Cleared after each use.
    std::vector<SolverVariable*> mVariablesToUpdate;

    /// ArrayRow.java:37 — public ArrayRowVariables variables
    /// Owning pointer to the variable storage implementation.
    /// Set by constructor (ArrayLinkedVariables) or by LinearSystem::ValuesRow (SolverVariableValues).
    std::unique_ptr<ArrayRowVariables> variables;

    // ─── Constructors ───

    /// ArrayRow.java:89 — public ArrayRow()
    ArrayRow();

    /// ArrayRow.java:92 — public ArrayRow(Cache cache)
    /// Creates the default ArrayLinkedVariables storage.
    explicit ArrayRow(Cache* cache);

    virtual ~ArrayRow() = default;

    // ─── Methods ───

    /// ArrayRow.java:97 — boolean hasKeyVariable()
    bool hasKeyVariable() const;

    /// ArrayRow.java:107 — public String toString()
    std::string toString() const;

    /// ArrayRow.java:111 — String toReadableString()
    std::string toReadableString() const;

    /// ArrayRow.java:165 — public void reset()
    void reset();

    /// ArrayRow.java:172 — boolean hasVariable(SolverVariable v)
    bool hasVariable(SolverVariable* v);

    /// ArrayRow.java:176 — ArrayRow createRowDefinition(SolverVariable variable, int value)
    ArrayRow* createRowDefinition(SolverVariable* variable, int value);

    /// ArrayRow.java:185 — public ArrayRow createRowEquals(SolverVariable variable, int value)
    ArrayRow* createRowEquals(SolverVariable* variable, int value);

    /// ArrayRow.java:197 — public ArrayRow createRowEquals(SolverVariable a, SolverVariable b, int margin)
    ArrayRow* createRowEquals(SolverVariable* variableA, SolverVariable* variableB, int margin);

    /// ArrayRow.java:220 — ArrayRow addSingleError(SolverVariable error, int sign)
    ArrayRow* addSingleError(SolverVariable* error, int sign);

    /// ArrayRow.java:226 — public ArrayRow createRowGreaterThan(a, b, slack, margin)
    ArrayRow* createRowGreaterThan(SolverVariable* variableA, SolverVariable* variableB,
                                   SolverVariable* slack, int margin);

    /// ArrayRow.java:251 — public ArrayRow createRowGreaterThan(a, b, slack)
    ArrayRow* createRowGreaterThan(SolverVariable* a, int b, SolverVariable* slack);

    /// ArrayRow.java:258 — public ArrayRow createRowLowerThan(a, b, slack, margin)
    ArrayRow* createRowLowerThan(SolverVariable* variableA, SolverVariable* variableB,
                                 SolverVariable* slack, int margin);

    /// ArrayRow.java:282 — public ArrayRow createRowEqualMatchDimensions(...)
    ArrayRow* createRowEqualMatchDimensions(float currentWeight, float totalWeights,
                                            float nextWeight,
                                            SolverVariable* variableStartA,
                                            SolverVariable* variableEndA,
                                            SolverVariable* variableStartB,
                                            SolverVariable* variableEndB);

    /// ArrayRow.java:320 — public ArrayRow createRowEqualDimension(...)
    ArrayRow* createRowEqualDimension(float currentWeight, float totalWeights, float nextWeight,
                                      SolverVariable* variableStartA, int marginStartA,
                                      SolverVariable* variableEndA, int marginEndA,
                                      SolverVariable* variableStartB, int marginStartB,
                                      SolverVariable* variableEndB, int marginEndB);

    /// ArrayRow.java:357 — ArrayRow createRowCentering(...)
    ArrayRow* createRowCentering(SolverVariable* variableA, SolverVariable* variableB,
                                  int marginA, float bias,
                                  SolverVariable* variableC, SolverVariable* variableD,
                                  int marginB);

    /// ArrayRow.java:406 — public ArrayRow addError(LinearSystem system, int strength)
    ArrayRow* addError(LinearSystem* system, int strength);

    /// ArrayRow.java:412 — ArrayRow createRowDimensionPercent(...)
    ArrayRow* createRowDimensionPercent(SolverVariable* variableA,
                                        SolverVariable* variableC, float percent);

    /// ArrayRow.java:430 — public ArrayRow createRowDimensionRatio(...)
    ArrayRow* createRowDimensionRatio(SolverVariable* variableA, SolverVariable* variableB,
                                      SolverVariable* variableC, SolverVariable* variableD,
                                      float ratio);

    /// ArrayRow.java:444 — public ArrayRow createRowWithAngle(...)
    ArrayRow* createRowWithAngle(SolverVariable* at, SolverVariable* ab,
                                  SolverVariable* bt, SolverVariable* bb,
                                  float angleComponent);

    /// ArrayRow.java:457 — int sizeInBytes()
    int sizeInBytes();

    /// ArrayRow.java:469 — void ensurePositiveConstant()
    void ensurePositiveConstant();

    /// ArrayRow.java:486 — boolean chooseSubject(LinearSystem system)
    bool chooseSubject(LinearSystem* system);

    /// ArrayRow.java:590 — void pivot(SolverVariable v)
    void pivot(SolverVariable* v);

    /// ArrayRow.java:720 — public SolverVariable pickPivot(SolverVariable exclude)
    SolverVariable* pickPivot(SolverVariable* exclude);

    // ─── Row interface implementation ───

    /// ArrayRow.java:610 — public boolean isEmpty()
    bool isEmpty() override;

    /// ArrayRow.java:615 — public void updateFromRow(...)
    void updateFromRow(LinearSystem* system, ArrayRow* definition, bool removeFromDefinition) override;

    /// ArrayRow.java:633 — public void updateFromFinalVariable(...)
    void updateFromFinalVariable(LinearSystem* system, SolverVariable* variable,
                                  bool removeFromDefinition) override;

    /// ArrayRow.java:653 — public void updateFromSynonymVariable(...)
    void updateFromSynonymVariable(LinearSystem* system, SolverVariable* variable,
                                    bool removeFromDefinition);

    /// ArrayRow.java:725 — public SolverVariable getPivotCandidate(...)
    SolverVariable* getPivotCandidate(LinearSystem* system, std::vector<bool>& avoid) override;

    /// ArrayRow.java:730 — public void clear()
    void clear() override;

    /// ArrayRow.java:740 — public void initFromRow(Row row)
    void initFromRow(Row* row) override;

    /// ArrayRow.java:754 — public void addError(SolverVariable error)
    void addError(SolverVariable* error) override;

    /// ArrayRow.java:771 — public SolverVariable getKey()
    SolverVariable* getKey() override;

    /// ArrayRow.java:776 — public void updateFromSystem(LinearSystem system)
    void updateFromSystem(LinearSystem* system) override;

private:
    /// ArrayRow.java:509 — SolverVariable chooseSubjectInVariables(LinearSystem system)
    SolverVariable* chooseSubjectInVariables(LinearSystem* system);

    /// ArrayRow.java:570 — private boolean isNew(SolverVariable variable, LinearSystem system)
    bool isNew(SolverVariable* variable, LinearSystem* system);

    /// ArrayRow.java:674 — private SolverVariable pickPivotInVariables(boolean[] avoid, SolverVariable exclude)
    SolverVariable* pickPivotInVariables(std::vector<bool>* avoid, SolverVariable* exclude);
};

} // namespace setu::cassowary
