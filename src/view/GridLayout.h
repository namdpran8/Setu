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
#include <vector>
#include <memory>

namespace setu {
namespace view {

class GridLayout : public ViewGroup {
public:
    enum Alignment { UNDEFINED, START, END, CENTER, FILL };

    struct Spec {
        int spanStart = -1;
        int spanSize = 1;
        float weight = 0.0f;
        Alignment alignment = UNDEFINED;
    };

    class LayoutParams : public View::LayoutParams {
    public:
        Spec rowSpec;
        Spec columnSpec;

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
        virtual ~LayoutParams() = default;
    };

    GridLayout();
    virtual ~GridLayout() = default;

    void setRowCount(int count);
    void setColumnCount(int count);

    int getRowCount() const { return mRowCount; }
    int getColumnCount() const { return mColumnCount; }

protected:
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;

private:
    int mRowCount = 0;
    int mColumnCount = 0;

    std::vector<int> mHorizontalGridLines; // Size = mColumnCount + 1
    std::vector<int> mVerticalGridLines;   // Size = mRowCount + 1
};

} // namespace view
} // namespace setu

