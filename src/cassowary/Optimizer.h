#pragma once

#include <vector>
#include <set>

namespace setu::cassowary {
    
class ConstraintWidgetContainer;
class ConstraintWidget;
class LinearSystem;

class Optimizer {
public:
    static constexpr int OPTIMIZATION_NONE = 0;
    static constexpr int OPTIMIZATION_STANDARD = 1;
    static constexpr int OPTIMIZATION_GRAPH = 2;
    static constexpr int OPTIMIZATION_GRAPH_WRAP = 4;
    static constexpr int OPTIMIZATION_DEPENDENCY_ORDERING = 8;
    
    static constexpr int FLAG_USE_OPTIMIZE = 0;
    static constexpr int FLAG_RECOMPUTE_BOUNDS = 1;

    inline static std::vector<bool> sFlags = {false, false, false};

    static void checkMatchParent(ConstraintWidgetContainer* container, LinearSystem* system, ConstraintWidget* widget) {
        // stub
    }

    static void addChildrenToSolverByDependency(ConstraintWidgetContainer* container, LinearSystem* system, std::set<ConstraintWidget*>& widgets, int orientation, bool addSelf) {
        // stub
    }
};

} // namespace setu::cassowary
