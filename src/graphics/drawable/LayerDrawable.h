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

#include "Drawable.h"
#include <vector>

namespace setu {
namespace graphics {

class LayerDrawable : public Drawable {
public:
    LayerDrawable() = default;

    void addLayer(const DrawablePtr& drawable, int insetLeft = 0, int insetTop = 0, int insetRight = 0, int insetBottom = 0);

    void draw(Canvas& canvas) override;
    
    int getIntrinsicWidth() const override;
    int getIntrinsicHeight() const override;
    bool getPadding(Rect& padding) const override;
    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }
    
    bool isStateful() const override;

protected:
    void onBoundsChange(const Rect& bounds) override;
    bool onStateChange(const std::vector<int>& stateSet) override;
    bool onLevelChange(int level) override;

private:
    struct Layer {
        DrawablePtr drawable;
        int insetLeft = 0;
        int insetTop = 0;
        int insetRight = 0;
        int insetBottom = 0;
    };
    
    std::vector<Layer> mLayers;
    int mAlpha = 255;
    Rect mPaddingInsets;
    bool mHasPadding = false;
};

} // namespace graphics
} // namespace setu
