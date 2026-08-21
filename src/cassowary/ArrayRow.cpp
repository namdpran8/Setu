// ArrayRow.cpp — Ported from androidx.constraintlayout.core.ArrayRow (ArrayRow.java)
// Original: ArrayRow.java:1-814
//
// Copyright (C) 2015 The Android Open Source Project — Apache 2.0

#include "ArrayRow.h"
#include "ArrayLinkedVariables.h"
#include "LinearSystem.h"
#include "Cache.h"

#include <cmath>
#include <sstream>

namespace setu::cassowary {

// ─── Constructors ───

/// ArrayRow.java:89 — public ArrayRow()
ArrayRow::ArrayRow() = default;

/// ArrayRow.java:92 — public ArrayRow(Cache cache)
ArrayRow::ArrayRow(Cache* cache) {
    variables = std::make_unique<ArrayLinkedVariables>(this, cache);
}

// ─── Methods ───

/// ArrayRow.java:97 — boolean hasKeyVariable()
bool ArrayRow::hasKeyVariable() const {
    return !(
        (mVariable == nullptr)
        || (mVariable->mType != SolverVariable::Type::UNRESTRICTED
            && mConstantValue < 0)
    );
}

/// ArrayRow.java:107 — public String toString()
std::string ArrayRow::toString() const {
    return toReadableString();
}

/// ArrayRow.java:111 — String toReadableString()
std::string ArrayRow::toReadableString() const {
    std::string s;
    if (mVariable == nullptr) {
        s += "0";
    } else {
        s += mVariable->toString();
    }
    s += " = ";
    bool addedVariable = false;
    if (mConstantValue != 0) {
        s += std::to_string(mConstantValue);
        addedVariable = true;
    }
    int count = variables->getCurrentSize();
    for (int i = 0; i < count; i++) {
        SolverVariable* v = variables->getVariable(i);
        if (v == nullptr) {
            continue;
        }
        float amount = variables->getVariableValue(i);
        if (amount == 0) {
            continue;
        }
        std::string name = v->toString();
        if (!addedVariable) {
            if (amount < 0) {
                s += "- ";
                amount *= -1;
            }
        } else {
            if (amount > 0) {
                s += " + ";
            } else {
                s += " - ";
                amount *= -1;
            }
        }
        if (amount == 1) {
            s += name;
        } else {
            s += std::to_string(amount) + " " + name;
        }
        addedVariable = true;
    }
    if (!addedVariable) {
        s += "0.0";
    }
    return s;
}

/// ArrayRow.java:165 — public void reset()
/// Named resetRow() to avoid collision with Row::clear(). Java uses the same name
/// for different semantics (reset resets internal state, clear is the Row interface method).
void ArrayRow::reset() {
    mVariable = nullptr;
    variables->clear();
    mConstantValue = 0;
    mIsSimpleDefinition = false;
}

/// ArrayRow.java:172 — boolean hasVariable(SolverVariable v)
bool ArrayRow::hasVariable(SolverVariable* v) {
    return variables->contains(v);
}

/// ArrayRow.java:176 — ArrayRow createRowDefinition(SolverVariable variable, int value)
ArrayRow* ArrayRow::createRowDefinition(SolverVariable* variable, int value) {
    this->mVariable = variable;
    variable->computedValue = static_cast<float>(value);
    mConstantValue = static_cast<float>(value);
    mIsSimpleDefinition = true;
    return this;
}

/// ArrayRow.java:185 — public ArrayRow createRowEquals(SolverVariable variable, int value)
ArrayRow* ArrayRow::createRowEquals(SolverVariable* variable, int value) {
    if (value < 0) {
        mConstantValue = static_cast<float>(-1 * value);
        variables->put(variable, 1);
    } else {
        mConstantValue = static_cast<float>(value);
        variables->put(variable, -1);
    }
    return this;
}

/// ArrayRow.java:197 — public ArrayRow createRowEquals(SolverVariable a, SolverVariable b, int margin)
ArrayRow* ArrayRow::createRowEquals(SolverVariable* variableA, SolverVariable* variableB, int margin) {
    bool inverse = false;
    if (margin != 0) {
        int m = margin;
        if (m < 0) {
            m = -1 * m;
            inverse = true;
        }
        mConstantValue = static_cast<float>(m);
    }
    if (!inverse) {
        variables->put(variableA, -1);
        variables->put(variableB, 1);
    } else {
        variables->put(variableA, 1);
        variables->put(variableB, -1);
    }
    return this;
}

/// ArrayRow.java:220 — ArrayRow addSingleError(SolverVariable error, int sign)
ArrayRow* ArrayRow::addSingleError(SolverVariable* error, int sign) {
    variables->put(error, static_cast<float>(sign));
    return this;
}

/// ArrayRow.java:226 — public ArrayRow createRowGreaterThan(a, b, slack, margin)
ArrayRow* ArrayRow::createRowGreaterThan(SolverVariable* variableA, SolverVariable* variableB,
                                          SolverVariable* slack, int margin) {
    bool inverse = false;
    if (margin != 0) {
        int m = margin;
        if (m < 0) {
            m = -1 * m;
            inverse = true;
        }
        mConstantValue = static_cast<float>(m);
    }
    if (!inverse) {
        variables->put(variableA, -1);
        variables->put(variableB, 1);
        variables->put(slack, 1);
    } else {
        variables->put(variableA, 1);
        variables->put(variableB, -1);
        variables->put(slack, -1);
    }
    return this;
}

/// ArrayRow.java:251 — public ArrayRow createRowGreaterThan(a, b, slack)
ArrayRow* ArrayRow::createRowGreaterThan(SolverVariable* a, int b, SolverVariable* slack) {
    mConstantValue = static_cast<float>(b);
    variables->put(a, -1);
    return this;
}

/// ArrayRow.java:258 — public ArrayRow createRowLowerThan(a, b, slack, margin)
ArrayRow* ArrayRow::createRowLowerThan(SolverVariable* variableA, SolverVariable* variableB,
                                        SolverVariable* slack, int margin) {
    bool inverse = false;
    if (margin != 0) {
        int m = margin;
        if (m < 0) {
            m = -1 * m;
            inverse = true;
        }
        mConstantValue = static_cast<float>(m);
    }
    if (!inverse) {
        variables->put(variableA, -1);
        variables->put(variableB, 1);
        variables->put(slack, -1);
    } else {
        variables->put(variableA, 1);
        variables->put(variableB, -1);
        variables->put(slack, 1);
    }
    return this;
}

/// ArrayRow.java:282 — public ArrayRow createRowEqualMatchDimensions(...)
ArrayRow* ArrayRow::createRowEqualMatchDimensions(float currentWeight, float totalWeights,
                                                    float nextWeight,
                                                    SolverVariable* variableStartA,
                                                    SolverVariable* variableEndA,
                                                    SolverVariable* variableStartB,
                                                    SolverVariable* variableEndB) {
    mConstantValue = 0;
    if (totalWeights == 0 || (currentWeight == nextWeight)) {
        variables->put(variableStartA, 1);
        variables->put(variableEndA, -1);
        variables->put(variableEndB, 1);
        variables->put(variableStartB, -1);
    } else {
        if (currentWeight == 0) {
            variables->put(variableStartA, 1);
            variables->put(variableEndA, -1);
        } else if (nextWeight == 0) {
            variables->put(variableStartB, 1);
            variables->put(variableEndB, -1);
        } else {
            float cw = currentWeight / totalWeights;
            float nw = nextWeight / totalWeights;
            float w = cw / nw;
            variables->put(variableStartA, 1);
            variables->put(variableEndA, -1);
            variables->put(variableEndB, w);
            variables->put(variableStartB, -w);
        }
    }
    return this;
}

/// ArrayRow.java:320 — public ArrayRow createRowEqualDimension(...)
ArrayRow* ArrayRow::createRowEqualDimension(float currentWeight, float totalWeights, float nextWeight,
                                              SolverVariable* variableStartA, int marginStartA,
                                              SolverVariable* variableEndA, int marginEndA,
                                              SolverVariable* variableStartB, int marginStartB,
                                              SolverVariable* variableEndB, int marginEndB) {
    if (totalWeights == 0 || (currentWeight == nextWeight)) {
        mConstantValue = static_cast<float>(-marginStartA - marginEndA + marginStartB + marginEndB);
        variables->put(variableStartA, 1);
        variables->put(variableEndA, -1);
        variables->put(variableEndB, 1);
        variables->put(variableStartB, -1);
    } else {
        float cw = currentWeight / totalWeights;
        float nw = nextWeight / totalWeights;
        float w = cw / nw;
        mConstantValue = -marginStartA - marginEndA + w * marginStartB + w * marginEndB;
        variables->put(variableStartA, 1);
        variables->put(variableEndA, -1);
        variables->put(variableEndB, w);
        variables->put(variableStartB, -w);
    }
    return this;
}

/// ArrayRow.java:357 — ArrayRow createRowCentering(...)
ArrayRow* ArrayRow::createRowCentering(SolverVariable* variableA, SolverVariable* variableB,
                                        int marginA, float bias,
                                        SolverVariable* variableC, SolverVariable* variableD,
                                        int marginB) {
    // ArrayRow.java:360 — object identity check (pointer comparison in C++)
    if (variableB == variableC) {
        variables->put(variableA, 1);
        variables->put(variableD, 1);
        variables->put(variableB, -2);
        return this;
    }
    if (bias == 0.5f) {
        variables->put(variableA, 1.0f);
        variables->put(variableB, -1.0f);
        variables->put(variableC, -1.0f);
        variables->put(variableD, 1.0f);
        if (marginA > 0 || marginB > 0) {
            mConstantValue = static_cast<float>(-marginA + marginB);
        }
    } else if (bias <= 0) {
        variables->put(variableA, -1);
        variables->put(variableB, 1);
        mConstantValue = static_cast<float>(marginA);
    } else if (bias >= 1) {
        variables->put(variableD, -1);
        variables->put(variableC, 1);
        mConstantValue = static_cast<float>(-marginB);
    } else {
        variables->put(variableA, 1.0f * (1.0f - bias));
        variables->put(variableB, -1.0f * (1.0f - bias));
        variables->put(variableC, -1.0f * bias);
        variables->put(variableD, 1.0f * bias);
        if (marginA > 0 || marginB > 0) {
            mConstantValue = -marginA * (1.0f - bias) + marginB * bias;
        }
    }
    return this;
}

/// ArrayRow.java:406 — public ArrayRow addError(LinearSystem system, int strength)
ArrayRow* ArrayRow::addError(LinearSystem* system, int strength) {
    variables->put(system->createErrorVariable(strength, "ep"), 1);
    variables->put(system->createErrorVariable(strength, "em"), -1);
    return this;
}

/// ArrayRow.java:412 — ArrayRow createRowDimensionPercent(...)
ArrayRow* ArrayRow::createRowDimensionPercent(SolverVariable* variableA,
                                                SolverVariable* variableC, float percent) {
    variables->put(variableA, -1);
    variables->put(variableC, percent);
    return this;
}

/// ArrayRow.java:430 — public ArrayRow createRowDimensionRatio(...)
ArrayRow* ArrayRow::createRowDimensionRatio(SolverVariable* variableA, SolverVariable* variableB,
                                              SolverVariable* variableC, SolverVariable* variableD,
                                              float ratio) {
    variables->put(variableA, -1);
    variables->put(variableB, 1);
    variables->put(variableC, ratio);
    variables->put(variableD, -ratio);
    return this;
}

/// ArrayRow.java:444 — public ArrayRow createRowWithAngle(...)
ArrayRow* ArrayRow::createRowWithAngle(SolverVariable* at, SolverVariable* ab,
                                        SolverVariable* bt, SolverVariable* bb,
                                        float angleComponent) {
    variables->put(bt, 0.5f);
    variables->put(bb, 0.5f);
    variables->put(at, -0.5f);
    variables->put(ab, -0.5f);
    mConstantValue = -angleComponent;
    return this;
}

/// ArrayRow.java:457 — int sizeInBytes()
int ArrayRow::sizeInBytes() {
    int size = 0;
    if (mVariable != nullptr) {
        size += 4;
    }
    size += 4; // constantValue
    size += 4; // used
    size += variables->sizeInBytes();
    return size;
}

/// ArrayRow.java:469 — void ensurePositiveConstant()
void ArrayRow::ensurePositiveConstant() {
    if (mConstantValue < 0) {
        mConstantValue *= -1;
        variables->invert();
    }
}

/// ArrayRow.java:486 — boolean chooseSubject(LinearSystem system)
bool ArrayRow::chooseSubject(LinearSystem* system) {
    bool addedExtra = false;
    SolverVariable* pivotCandidate = chooseSubjectInVariables(system);
    if (pivotCandidate == nullptr) {
        addedExtra = true;
    } else {
        pivot(pivotCandidate);
    }
    if (variables->getCurrentSize() == 0) {
        mIsSimpleDefinition = true;
    }
    return addedExtra;
}

/// ArrayRow.java:509 — SolverVariable chooseSubjectInVariables(LinearSystem system)
SolverVariable* ArrayRow::chooseSubjectInVariables(LinearSystem* system) {
    SolverVariable* restrictedCandidate = nullptr;
    SolverVariable* unrestrictedCandidate = nullptr;
    float unrestrictedCandidateAmount = 0;
    float restrictedCandidateAmount = 0;
    bool unrestrictedCandidateIsNew = false;
    bool restrictedCandidateIsNew = false;

    const int currentSize = variables->getCurrentSize();
    for (int i = 0; i < currentSize; i++) {
        float amount = variables->getVariableValue(i);
        SolverVariable* variable = variables->getVariable(i);
        if (variable->mType == SolverVariable::Type::UNRESTRICTED) {
            if (unrestrictedCandidate == nullptr) {
                unrestrictedCandidate = variable;
                unrestrictedCandidateAmount = amount;
                unrestrictedCandidateIsNew = isNew(variable, system);
            } else if (unrestrictedCandidateAmount > amount) {
                unrestrictedCandidate = variable;
                unrestrictedCandidateAmount = amount;
                unrestrictedCandidateIsNew = isNew(variable, system);
            } else if (!unrestrictedCandidateIsNew && isNew(variable, system)) {
                unrestrictedCandidate = variable;
                unrestrictedCandidateAmount = amount;
                unrestrictedCandidateIsNew = true;
            }
        } else if (unrestrictedCandidate == nullptr) {
            if (amount < 0) {
                if (restrictedCandidate == nullptr) {
                    restrictedCandidate = variable;
                    restrictedCandidateAmount = amount;
                    restrictedCandidateIsNew = isNew(variable, system);
                } else if (restrictedCandidateAmount > amount) {
                    restrictedCandidate = variable;
                    restrictedCandidateAmount = amount;
                    restrictedCandidateIsNew = isNew(variable, system);
                } else if (!restrictedCandidateIsNew && isNew(variable, system)) {
                    restrictedCandidate = variable;
                    restrictedCandidateAmount = amount;
                    restrictedCandidateIsNew = true;
                }
            }
        }
    }

    if (unrestrictedCandidate != nullptr) {
        return unrestrictedCandidate;
    }
    return restrictedCandidate;
}

/// ArrayRow.java:570 — private boolean isNew(SolverVariable variable, LinearSystem system)
bool ArrayRow::isNew(SolverVariable* variable, LinearSystem* system) {
    // FULL_NEW_CHECK is false; using fast path only (usage count check).
    // ArrayRow.java:587 — return variable.usageInRowCount <= 1;
    return variable->usageInRowCount <= 1;
}

/// ArrayRow.java:590 — void pivot(SolverVariable v)
void ArrayRow::pivot(SolverVariable* v) {
    if (mVariable != nullptr) {
        variables->put(mVariable, -1.0f);
        mVariable->mDefinitionId = -1;
        mVariable = nullptr;
    }

    float amount = variables->remove(v, true) * -1;
    mVariable = v;
    if (amount == 1) {
        return;
    }
    mConstantValue = mConstantValue / amount;
    variables->divideByAmount(amount);
}

/// ArrayRow.java:674 — private SolverVariable pickPivotInVariables(boolean[] avoid, SolverVariable exclude)
SolverVariable* ArrayRow::pickPivotInVariables(std::vector<bool>* avoid, SolverVariable* exclude) {
    bool all = true;
    float value = 0;
    SolverVariable* pivotVar = nullptr;
    SolverVariable* pivotSlack = nullptr;
    float valueSlack = 0;

    const int currentSize = variables->getCurrentSize();
    for (int i = 0; i < currentSize; i++) {
        float currentValue = variables->getVariableValue(i);
        if (currentValue < 0) {
            SolverVariable* v = variables->getVariable(i);
            if (!((avoid != nullptr && v->id < static_cast<int>(avoid->size()) && (*avoid)[v->id])
                  || (v == exclude))) {
                if (all) {
                    if (v->mType == SolverVariable::Type::SLACK
                        || v->mType == SolverVariable::Type::ERROR) {
                        if (currentValue < value) {
                            value = currentValue;
                            pivotVar = v;
                        }
                    }
                } else {
                    if (v->mType == SolverVariable::Type::SLACK) {
                        if (currentValue < valueSlack) {
                            valueSlack = currentValue;
                            pivotSlack = v;
                        }
                    } else if (v->mType == SolverVariable::Type::ERROR) {
                        if (currentValue < value) {
                            value = currentValue;
                            pivotVar = v;
                        }
                    }
                }
            }
        }
    }
    if (all) {
        return pivotVar;
    }
    return pivotVar != nullptr ? pivotVar : pivotSlack;
}

/// ArrayRow.java:720 — public SolverVariable pickPivot(SolverVariable exclude)
SolverVariable* ArrayRow::pickPivot(SolverVariable* exclude) {
    return pickPivotInVariables(nullptr, exclude);
}

// ─── Row interface implementation ───

/// ArrayRow.java:610 — public boolean isEmpty()
bool ArrayRow::isEmpty() {
    return (mVariable == nullptr && mConstantValue == 0 && variables->getCurrentSize() == 0);
}

/// ArrayRow.java:615 — public void updateFromRow(...)
void ArrayRow::updateFromRow(LinearSystem* system, ArrayRow* definition, bool removeFromDefinition) {
    float value = variables->use(definition, removeFromDefinition);
    mConstantValue += definition->mConstantValue * value;
    if (removeFromDefinition) {
        definition->mVariable->removeFromRow(this);
    }
    if (LinearSystem::SIMPLIFY_SYNONYMS
        && mVariable != nullptr && variables->getCurrentSize() == 0) {
        mIsSimpleDefinition = true;
        system->hasSimpleDefinition = true;
    }
}

/// ArrayRow.java:633 — public void updateFromFinalVariable(...)
void ArrayRow::updateFromFinalVariable(LinearSystem* system, SolverVariable* variable,
                                        bool removeFromDefinition) {
    if (variable == nullptr || !variable->isFinalValue) {
        return;
    }
    float value = variables->get(variable);
    mConstantValue += variable->computedValue * value;
    variables->remove(variable, removeFromDefinition);
    if (removeFromDefinition) {
        variable->removeFromRow(this);
    }
    if (LinearSystem::SIMPLIFY_SYNONYMS
        && variables->getCurrentSize() == 0) {
        mIsSimpleDefinition = true;
        system->hasSimpleDefinition = true;
    }
}

/// ArrayRow.java:653 — public void updateFromSynonymVariable(...)
void ArrayRow::updateFromSynonymVariable(LinearSystem* system, SolverVariable* variable,
                                          bool removeFromDefinition) {
    if (variable == nullptr || !variable->mIsSynonym) {
        return;
    }
    float value = variables->get(variable);
    mConstantValue += variable->mSynonymDelta * value;
    variables->remove(variable, removeFromDefinition);
    if (removeFromDefinition) {
        variable->removeFromRow(this);
    }
    // ArrayRow.java:665 — system.mCache.mIndexedVariables[variable.mSynonym]
    // Access the synonym target through the system's cache.
    variables->add(system->getCache()->mIndexedVariables[variable->mSynonym],
                   value, removeFromDefinition);
    if (LinearSystem::SIMPLIFY_SYNONYMS
        && variables->getCurrentSize() == 0) {
        mIsSimpleDefinition = true;
        system->hasSimpleDefinition = true;
    }
}

/// ArrayRow.java:725 — public SolverVariable getPivotCandidate(...)
SolverVariable* ArrayRow::getPivotCandidate(LinearSystem* system, std::vector<bool>& avoid) {
    return pickPivotInVariables(&avoid, nullptr);
}

/// ArrayRow.java:730 — public void clear()
void ArrayRow::clear() {
    variables->clear();
    mVariable = nullptr;
    mConstantValue = 0;
}

/// ArrayRow.java:740 — public void initFromRow(Row row)
void ArrayRow::initFromRow(Row* row) {
    // Java: if (row instanceof ArrayRow)
    auto* copiedRow = dynamic_cast<ArrayRow*>(row);
    if (copiedRow != nullptr) {
        mVariable = nullptr;
        variables->clear();
        for (int i = 0; i < copiedRow->variables->getCurrentSize(); i++) {
            SolverVariable* var = copiedRow->variables->getVariable(i);
            float val = copiedRow->variables->getVariableValue(i);
            variables->add(var, val, true);
        }
    }
}

/// ArrayRow.java:754 — public void addError(SolverVariable error)
void ArrayRow::addError(SolverVariable* error) {
    float weight = 1;
    if (error->strength == SolverVariable::STRENGTH_LOW) {
        weight = 1.0f;
    } else if (error->strength == SolverVariable::STRENGTH_MEDIUM) {
        weight = 1E3f;
    } else if (error->strength == SolverVariable::STRENGTH_HIGH) {
        weight = 1E6f;
    } else if (error->strength == SolverVariable::STRENGTH_HIGHEST) {
        weight = 1E9f;
    } else if (error->strength == SolverVariable::STRENGTH_EQUALITY) {
        weight = 1E12f;
    }
    variables->put(error, weight);
}

/// ArrayRow.java:771 — public SolverVariable getKey()
SolverVariable* ArrayRow::getKey() {
    return mVariable;
}

/// ArrayRow.java:776 — public void updateFromSystem(LinearSystem system)
void ArrayRow::updateFromSystem(LinearSystem* system) {


    bool done = false;
    while (!done) {
        int currentSize = variables->getCurrentSize();
        for (int i = 0; i < currentSize; i++) {
            SolverVariable* variable = variables->getVariable(i);
            if (variable->mDefinitionId != -1 || variable->isFinalValue || variable->mIsSynonym) {
                mVariablesToUpdate.push_back(variable);
            }
        }
        const int size = static_cast<int>(mVariablesToUpdate.size());
        if (size > 0) {
            for (int i = 0; i < size; i++) {
                SolverVariable* variable = mVariablesToUpdate[i];
                if (variable->isFinalValue) {
                    updateFromFinalVariable(system, variable, true);
                } else if (variable->mIsSynonym) {
                    updateFromSynonymVariable(system, variable, true);
                } else {
                    updateFromRow(system, system->getRow(variable->mDefinitionId), true);
                }
            }
            mVariablesToUpdate.clear();
        } else {
            done = true;
        }
    }
    if (LinearSystem::SIMPLIFY_SYNONYMS
        && mVariable != nullptr && variables->getCurrentSize() == 0) {
        mIsSimpleDefinition = true;
        system->hasSimpleDefinition = true;
    }
}

} // namespace setu::cassowary
