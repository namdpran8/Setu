// SolverVariable.h — Ported from androidx.constraintlayout.core.SolverVariable (SolverVariable.java)
// Original: SolverVariable.java:1-344
//
// Represents a variable in the linear constraint system.
//
// OWNERSHIP MODEL:
// - SolverVariable instances are allocated by LinearSystem and recycled through Cache's
//   SimplePool<SolverVariable>. LinearSystem owns all SolverVariable lifetimes.
// - mClientEquations[] stores raw non-owning pointers to ArrayRow objects.
// - When LinearSystem::reset() is called, it first calls reset() on every variable
//   (clearing mClientEquations[]), then releases rows. This ordering ensures no
//   recycled SolverVariable retains stale row pointers.
// - VAR_USE_HASH is false in AOSP; the array-based path (mClientEquations) is used.
//   The HashSet path is not ported (dead code).
//
// Copyright (C) 2015 The Android Open Source Project — Apache 2.0

#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

namespace setu::cassowary {

// Forward declarations
class ArrayRow;
class LinearSystem;

/// Ported from SolverVariable.java:27 — public class SolverVariable
class SolverVariable {
public:
    // ─── SolverVariable.java:77-98 — Type enum ───
    enum class Type {
        UNRESTRICTED,
        CONSTANT,
        SLACK,
        ERROR,
        UNKNOWN
    };

    // ─── SolverVariable.java:35-44 — Strength constants ───
    static constexpr int STRENGTH_NONE     = 0;
    static constexpr int STRENGTH_LOW      = 1;
    static constexpr int STRENGTH_MEDIUM   = 2;
    static constexpr int STRENGTH_HIGH     = 3;
    static constexpr int STRENGTH_HIGHEST  = 4;
    static constexpr int STRENGTH_EQUALITY = 5;
    static constexpr int STRENGTH_BARRIER  = 6;
    static constexpr int STRENGTH_CENTERING = 7;
    static constexpr int STRENGTH_FIXED    = 8;

    // ─── SolverVariable.java:61 — MAX_STRENGTH ───
    static constexpr int MAX_STRENGTH = 9;

    // ─── SolverVariable.java:46-50 — static ID counters ───
    // These are global counters used for debug naming only (INTERNAL_DEBUG).
    // In release builds they are unused, but we keep them for fidelity.
    static inline int sUniqueSlackId = 1;
    static inline int sUniqueErrorId = 1;
    static inline int sUniqueUnrestrictedId = 1;
    static inline int sUniqueConstantId = 1;
    static inline int sUniqueId = 1;

    // ─── SolverVariable.java:29-31 — Debug flags ───
    // FULL_DEBUG is defined in LinearSystem; we mirror it here as false.
    static constexpr bool INTERNAL_DEBUG = false;
    // VAR_USE_HASH = false in AOSP — HashSet<ArrayRow> path is dead code, not ported.
    static constexpr bool VAR_USE_HASH = false;
    static constexpr bool DO_NOT_USE = false;

    // ─── SolverVariable.java:51-72 — Public fields ───
    bool inGoal = false;
    int id = -1;
    int mDefinitionId = -1;
    int strength = 0;
    float computedValue = 0.0f;
    bool isFinalValue = false;

    float mStrengthVector[MAX_STRENGTH] = {};
    float mGoalStrengthVector[MAX_STRENGTH] = {};

    Type mType = Type::UNKNOWN;

    // ─── SolverVariable.java:67-69 — Client equations (row back-references) ───
    // Raw non-owning pointers. Cleared on reset(). See ownership note at top of file.
    // Initial capacity 16 matches Java. reserve() called in constructor.
    std::vector<ArrayRow*> mClientEquations;
    int mClientEquationsCount = 0;
    int usageInRowCount = 0;

    // ─── SolverVariable.java:70-72 — Synonym support ───
    bool mIsSynonym = false;
    int mSynonym = -1;
    float mSynonymDelta = 0.0f;

    // ─── Constructors ───

    /// SolverVariable.java:130 — SolverVariable(String name, Type type)
    SolverVariable(const std::string& name, Type type);

    /// SolverVariable.java:135 — SolverVariable(Type type, String prefix)
    SolverVariable(Type type, const std::string& prefix = "");

    /// Default constructor for pool pre-allocation
    SolverVariable();

    // ─── Methods ───

    /// SolverVariable.java:100 — static void increaseErrorId()
    static void increaseErrorId();

    /// SolverVariable.java:142 — void clearStrengths()
    void clearStrengths();

    /// SolverVariable.java:148 — String strengthsToString()
    std::string strengthsToString() const;

    /// SolverVariable.java:181 — public final void addToRow(ArrayRow row)
    /// Registers that this variable appears in the given row.
    /// Uses the array-based path (VAR_USE_HASH=false).
    void addToRow(ArrayRow* row);

    /// SolverVariable.java:199 — public final void removeFromRow(ArrayRow row)
    /// Unregisters a row from this variable's client list.
    void removeFromRow(ArrayRow* row);

    /// SolverVariable.java:217 — public final void updateReferencesWithNewDefinition(...)
    /// When this variable gets a new defining row, update all other rows that reference it.
    void updateReferencesWithNewDefinition(LinearSystem* system, ArrayRow* definition);

    /// SolverVariable.java:233 — public void setFinalValue(LinearSystem system, float value)
    void setFinalValue(LinearSystem* system, float value);

    /// SolverVariable.java:251 — public void setSynonym(...)
    void setSynonym(LinearSystem* system, SolverVariable* synonymVariable, float value);

    /// SolverVariable.java:268 — public void reset()
    /// Resets the variable for reuse from the pool.
    /// SAFETY: Nulls out all mClientEquations entries before resetting count,
    /// ensuring no stale row pointers survive recycling.
    void reset();

    /// SolverVariable.java:298 — public String getName()
    const std::string& getName() const;

    /// SolverVariable.java:302 — public void setName(String name)
    void setName(const std::string& name);

    /// SolverVariable.java:307 — public void setType(Type type, String prefix)
    void setType(Type type, const std::string& prefix = "");

    /// SolverVariable.java:315 — public int compareTo(SolverVariable v)
    /// Used by std::sort in PriorityGoalRow. Provided as operator< for C++ idiom.
    bool operator<(const SolverVariable& other) const;

    /// SolverVariable.java:323 — public String toString()
    std::string toString() const;

private:
    std::string mName;

    /// SolverVariable.java:104 — private static String getUniqueName(Type, String)
    static std::string getUniqueName(Type type, const std::string& prefix);
};

} // namespace setu::cassowary
