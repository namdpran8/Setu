#pragma once

#include "ConstraintWidget.h"
#include <set>

#include "Guideline.h"

#include "Barrier.h"

namespace setu::cassowary {

class VirtualLayout : public ConstraintWidget {
public:
    virtual ~VirtualLayout() = default;
    bool contains(const std::set<ConstraintWidget*>& widgets) { return false; }
};

class ChainHead {
public:
    ChainHead(ConstraintWidget* first, int orientation, bool isRtl) {}
    virtual ~ChainHead() = default;
    void addToSolver(LinearSystem* system) {}
};

} // namespace setu::cassowary
