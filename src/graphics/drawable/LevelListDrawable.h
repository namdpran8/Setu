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

class LevelListDrawable : public Drawable {
public:
    LevelListDrawable() = default;

    void addLevel(int minLevel, int maxLevel, const DrawablePtr& drawable);

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
    struct LevelItem {
        int minLevel;
        int maxLevel;
        DrawablePtr drawable;
    };
    
    std::vector<LevelItem> mItems;
    DrawablePtr mCurrentDrawable;
    int mAlpha = 255;
    
    void updateCurrentDrawable(int level);
};

} // namespace graphics
} // namespace setu
