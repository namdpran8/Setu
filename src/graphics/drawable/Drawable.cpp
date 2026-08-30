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

#include "Drawable.h"

namespace setu {
namespace graphics {

// If you\'re reading this, you\'re probably trying to understand what this does.
// Good luck. The comment above was written by AI because I didn\'t want to
// explain it myself. I hope it was correct.
void Drawable::setBounds(int left, int top, int right, int bottom) {
    if (mBounds.left == left && mBounds.top == top &&
        mBounds.right == right && mBounds.bottom == bottom) {
        return;
    }
    // AOSP only fires onBoundsChange when the bounds were non-empty before or
    // after; the guard above already covers the no-change case, and subclasses
    // are cheap to notify, so notify unconditionally.
    mBounds.set(left, top, right, bottom);
    onBoundsChange(mBounds);
}

void Drawable::invalidateSelf() {
    if (mCallback) {
        mCallback->invalidateDrawable(this);
    }
}

void Drawable::scheduleSelf(std::function<void()> what, long long whenMs) {
    if (mCallback) {
        mCallback->scheduleDrawable(this, std::move(what), whenMs);
    }
}

void Drawable::unscheduleSelf() {
    if (mCallback) {
        mCallback->unscheduleDrawable(this);
    }
}

bool Drawable::setState(const std::vector<int>& stateSet) {
    if (mStateSet == stateSet) return false;
    mStateSet = stateSet;
    return onStateChange(stateSet);
}

bool Drawable::setLevel(int level) {
    if (mLevel == level) return false;
    mLevel = level;
    return onLevelChange(level);
}

bool Drawable::getPadding(Rect& padding) const {
    padding.setEmpty();
    return false;
}

bool Drawable::setVisible(bool visible, bool restart) {
    const bool changed = (mVisible != visible);
    if (changed) {
        mVisible = visible;
        invalidateSelf();
    }
    return changed;
}

} // namespace graphics
} // namespace setu
