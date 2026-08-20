// SolverVariable.cpp — Ported from androidx.constraintlayout.core.SolverVariable (SolverVariable.java)
// Original: SolverVariable.java:1-344
//
// Copyright (C) 2015 The Android Open Source Project — Apache 2.0

#include "SolverVariable.h"
#include "ArrayRow.h"
#include "LinearSystem.h"

#include <sstream>
#include <cassert>

namespace setu::cassowary {

// ─── Constructors ───

/// SolverVariable.java:130 — SolverVariable(String name, Type type)
SolverVariable::SolverVariable(const std::string& name, Type type)
    : mName(name), mType(type) {
    mClientEquations.resize(16, nullptr);
}

/// SolverVariable.java:135 — SolverVariable(Type type, String prefix)
SolverVariable::SolverVariable(Type type, const std::string& prefix)
    : mType(type) {
    mClientEquations.resize(16, nullptr);
    // Java: if (INTERNAL_DEBUG) { mName = getUniqueName(type, prefix); }
    // INTERNAL_DEBUG is false in release; name assignment is dead code.
}

/// Default constructor for pool pre-allocation
SolverVariable::SolverVariable()
    : mType(Type::UNKNOWN) {
    mClientEquations.resize(16, nullptr);
}

// ─── Static methods ───

/// SolverVariable.java:100 — static void increaseErrorId()
void SolverVariable::increaseErrorId() {
    sUniqueErrorId++;
}

/// SolverVariable.java:104 — private static String getUniqueName(Type type, String prefix)
std::string SolverVariable::getUniqueName(Type type, const std::string& prefix) {
    if (!prefix.empty()) {
        return prefix + std::to_string(sUniqueErrorId);
    }
    switch (type) {
        case Type::UNRESTRICTED: return "U" + std::to_string(++sUniqueUnrestrictedId);
        case Type::CONSTANT:     return "C" + std::to_string(++sUniqueConstantId);
        case Type::SLACK:        return "S" + std::to_string(++sUniqueSlackId);
        case Type::ERROR:        return "e" + std::to_string(++sUniqueErrorId);
        case Type::UNKNOWN:      return "V" + std::to_string(++sUniqueId);
    }
    // SolverVariable.java:121 — throw new AssertionError(type.name())
    assert(false && "Unknown SolverVariable::Type");
    return "?";
}

// ─── Instance methods ───

/// SolverVariable.java:142 — void clearStrengths()
void SolverVariable::clearStrengths() {
    for (int i = 0; i < MAX_STRENGTH; i++) {
        mStrengthVector[i] = 0;
    }
}

/// SolverVariable.java:148 — String strengthsToString()
std::string SolverVariable::strengthsToString() const {
    std::string representation = toString() + "[";
    bool negative = false;
    bool empty = true;
    for (int j = 0; j < MAX_STRENGTH; j++) {
        representation += std::to_string(mStrengthVector[j]);
        if (mStrengthVector[j] > 0) {
            negative = false;
        } else if (mStrengthVector[j] < 0) {
            negative = true;
        }
        if (mStrengthVector[j] != 0) {
            empty = false;
        }
        if (j < MAX_STRENGTH - 1) {
            representation += ", ";
        } else {
            representation += "] ";
        }
    }
    if (negative) {
        representation += " (-)";
    }
    if (empty) {
        representation += " (*)";
    }
    return representation;
}

/// SolverVariable.java:181 — public final void addToRow(ArrayRow row)
void SolverVariable::addToRow(ArrayRow* row) {
    // VAR_USE_HASH is false; using array-based path only.
    // Check for duplicates (linear scan, same as Java)
    for (int i = 0; i < mClientEquationsCount; i++) {
        if (mClientEquations[i] == row) {
            return;
        }
    }
    // Grow if needed — Java: Arrays.copyOf(mClientEquations, length * 2)
    if (mClientEquationsCount >= static_cast<int>(mClientEquations.size())) {
        mClientEquations.resize(mClientEquations.size() * 2, nullptr);
    }
    mClientEquations[mClientEquationsCount] = row;
    mClientEquationsCount++;
}

/// SolverVariable.java:199 — public final void removeFromRow(ArrayRow row)
void SolverVariable::removeFromRow(ArrayRow* row) {
    // VAR_USE_HASH is false; using array-based path only.
    const int count = mClientEquationsCount;
    for (int i = 0; i < count; i++) {
        if (mClientEquations[i] == row) {
            // Shift remaining elements left
            for (int j = i; j < count - 1; j++) {
                mClientEquations[j] = mClientEquations[j + 1];
            }
            mClientEquationsCount--;
            return;
        }
    }
}

/// SolverVariable.java:217 — public final void updateReferencesWithNewDefinition(...)
void SolverVariable::updateReferencesWithNewDefinition(LinearSystem* system, ArrayRow* definition) {
    // VAR_USE_HASH is false; using array-based path only.
    const int count = mClientEquationsCount;
    for (int i = 0; i < count; i++) {
        mClientEquations[i]->updateFromRow(system, definition, false);
    }
    mClientEquationsCount = 0;
}

/// SolverVariable.java:233 — public void setFinalValue(LinearSystem system, float value)
void SolverVariable::setFinalValue(LinearSystem* system, float value) {
    // DO_NOT_USE && INTERNAL_DEBUG is false; debug print is dead code.
    computedValue = value;
    isFinalValue = true;
    mIsSynonym = false;
    mSynonym = -1;
    mSynonymDelta = 0;
    const int count = mClientEquationsCount;
    mDefinitionId = -1;
    for (int i = 0; i < count; i++) {
        mClientEquations[i]->updateFromFinalVariable(system, this, false);
    }
    mClientEquationsCount = 0;
}

/// SolverVariable.java:251 — public void setSynonym(...)
void SolverVariable::setSynonym(LinearSystem* system, SolverVariable* synonymVariable, float value) {
    // INTERNAL_DEBUG is false; debug print is dead code.
    mIsSynonym = true;
    mSynonym = synonymVariable->id;
    mSynonymDelta = value;
    const int count = mClientEquationsCount;
    mDefinitionId = -1;
    for (int i = 0; i < count; i++) {
        mClientEquations[i]->updateFromSynonymVariable(system, this, false);
    }
    mClientEquationsCount = 0;
    // SolverVariable.java:264 — system.displayReadableRows()
    // In Java this is unconditional but is a no-op when DEBUG=false.
    // We keep the call for fidelity; LinearSystem::displayReadableRows()
    // checks the DEBUG flag internally.
    system->displayReadableRows();
}

/// SolverVariable.java:268 — public void reset()
void SolverVariable::reset() {
    mName.clear();
    mType = Type::UNKNOWN;
    strength = STRENGTH_NONE;
    id = -1;
    mDefinitionId = -1;
    computedValue = 0;
    isFinalValue = false;
    mIsSynonym = false;
    mSynonym = -1;
    mSynonymDelta = 0;
    // SAFETY: Null out all client equation pointers before resetting count.
    // This ensures no stale row pointers survive if this variable is recycled
    // from the pool while a row hasn't been released yet (shouldn't happen
    // per LinearSystem::reset() ordering, but belt-and-suspenders).
    const int count = mClientEquationsCount;
    for (int i = 0; i < count; i++) {
        mClientEquations[i] = nullptr;
    }
    mClientEquationsCount = 0;
    usageInRowCount = 0;
    inGoal = false;
    std::fill(std::begin(mGoalStrengthVector), std::end(mGoalStrengthVector), 0.0f);
}

/// SolverVariable.java:298 — public String getName()
const std::string& SolverVariable::getName() const {
    return mName;
}

/// SolverVariable.java:302 — public void setName(String name)
void SolverVariable::setName(const std::string& name) {
    mName = name;
}

/// SolverVariable.java:307 — public void setType(Type type, String prefix)
void SolverVariable::setType(Type type, const std::string& prefix) {
    mType = type;
    // Java: if (INTERNAL_DEBUG && mName == null) { mName = getUniqueName(type, prefix); }
    // INTERNAL_DEBUG is false; dead code in release.
}

/// SolverVariable.java:315 — public int compareTo(SolverVariable v)
/// Deviation: Java Comparable uses compareTo returning int; C++ uses operator< for std::sort.
/// Semantics are equivalent: sorts by ascending id.
bool SolverVariable::operator<(const SolverVariable& other) const {
    return this->id < other.id;
}

/// SolverVariable.java:323 — public String toString()
std::string SolverVariable::toString() const {
    std::string result;
    if constexpr (INTERNAL_DEBUG) {
        result += mName + "(" + std::to_string(id) + "):" + std::to_string(strength);
        if (mIsSynonym) {
            result += ":S(" + std::to_string(mSynonym) + ")";
        }
        if (isFinalValue) {
            result += ":F(" + std::to_string(computedValue) + ")";
        }
    } else {
        if (!mName.empty()) {
            result += mName;
        } else {
            result += std::to_string(id);
        }
    }
    return result;
}

} // namespace setu::cassowary
