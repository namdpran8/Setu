/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
