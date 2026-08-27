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

class FrameLayout : public ViewGroup {
public:
    class LayoutParams : public View::LayoutParams {
    public:
        int gravity = -1; // -1 means top|left usually

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
    };

    FrameLayout() = default;
    virtual ~FrameLayout() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;
};

} // namespace view
} // namespace setu

