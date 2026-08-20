// ConstraintAnchor.h — Stub for Tier 2 (will be fully ported in Step 4)
// Provides minimal declarations needed by LinearSystem during Tier 1 compilation.
//
// TODO(step4): Replace this stub with the full ConstraintAnchor port.

#pragma once

#include "SolverVariable.h"
#include <string>

namespace setu::cassowary {

class Cache;

/// Stub — Tier 2 port pending.
/// Anchor type enum matching Java ConstraintAnchor.Type
class ConstraintAnchor {
public:
    enum class Type {
        NONE,
        LEFT,
        TOP,
        RIGHT,
        BOTTOM,
        BASELINE,
        CENTER,
        CENTER_X,
        CENTER_Y
    };

    // Methods called by LinearSystem::createObjectVariable
    SolverVariable* getSolverVariable() { return &mSolverVariable; }
    void resetSolverVariable(Cache* /*cache*/) { mSolverVariable.reset(); }

    // Methods called by LinearSystem::getObjectVariableValue
    bool hasFinalValue() const { return false; }
    int getFinalValue() const { return 0; }

private:
    SolverVariable mSolverVariable;
};

} // namespace setu::cassowary
