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

#include "ViewGroup.h"

namespace setu {
namespace view {

// A foundational stacking container for panel widgets. It deliberately has no
// gesture policy: all children share the available bounds and later children
// are drawn and hit-tested above earlier children.
class OverlayPanelLayout : public ViewGroup {
public:
    OverlayPanelLayout() = default;
    ~OverlayPanelLayout() override = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;
    std::string getClassName() const override { return "OverlayPanelLayout"; }
};

} // namespace view
} // namespace setu
