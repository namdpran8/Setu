#pragma once

#include "ConstraintWidget.h"
#include <set>

namespace setu::cassowary {
    
class Guideline : public ConstraintWidget {
public:
    static constexpr int HORIZONTAL = 0;
    static constexpr int VERTICAL = 1;
    virtual ~Guideline() = default;
    int getOrientation() const { return HORIZONTAL; }
};

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
