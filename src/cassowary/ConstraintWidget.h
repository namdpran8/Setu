// ConstraintWidget.h — Stub for Tier 2 (will be fully ported in Step 4)
// Provides minimal declarations needed by LinearSystem::addCenterPoint during Tier 1 compilation.
//
// TODO(step4): Replace this stub with the full ConstraintWidget port.

#pragma once

#include "ConstraintAnchor.h"

namespace setu::cassowary {

/// Stub — Tier 2 port pending.
class ConstraintWidget {
public:
    // Called by LinearSystem::addCenterPoint
    ConstraintAnchor* getAnchor(ConstraintAnchor::Type /*type*/) {
        // TODO(step4): Implement real anchor lookup
        return nullptr;
    }
};

} // namespace setu::cassowary
