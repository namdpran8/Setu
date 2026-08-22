#pragma once

#include "ConstraintWidget.h"
#include <set>

#include "Guideline.h"

namespace setu::cassowary {

class Barrier : public ConstraintWidget {
public:
    virtual ~Barrier() = default;
    void markWidgets() {}
};

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
