#pragma once

#include <vector>

namespace setu::cassowary {

class ConstraintWidgetContainer;
class LinearSystem;
class ChainHead;
class ConstraintWidget;

class Chain {
public:
    static constexpr bool DEBUG = false;
    static constexpr bool USE_CHAIN_OPTIMIZATION = false;

    static void applyChainConstraints(ConstraintWidgetContainer* constraintWidgetContainer, LinearSystem* system, std::vector<ChainHead*>& chains, int chainsSize, int orientation);

private:
    static void applyChainConstraints(ConstraintWidgetContainer* container, LinearSystem* system, int orientation, int offset, ChainHead* chainHead);
};

} // namespace setu::cassowary
