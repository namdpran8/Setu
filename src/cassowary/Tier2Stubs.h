#pragma once

#include "ConstraintWidget.h"
#include <set>

#include "Guideline.h"

#include "Barrier.h"

#include "ChainHead.h"

namespace setu::cassowary {

class VirtualLayout : public ConstraintWidget {
public:
    virtual ~VirtualLayout() = default;
    bool contains(const std::set<ConstraintWidget*>& widgets) { return false; }
};

} // namespace setu::cassowary
