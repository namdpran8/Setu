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

#include "LevelListDrawable.h"

namespace setu {
namespace graphics {

void LevelListDrawable::addLevel(int minLevel, int maxLevel, const DrawablePtr& drawable) {
    if (drawable) {
        mItems.push_back({minLevel, maxLevel, drawable});
        // Initial setup
        updateCurrentDrawable(getLevel());
    }
}

void LevelListDrawable::draw(Canvas& canvas) {
    if (mCurrentDrawable) {
        mCurrentDrawable->draw(canvas);
    }
}

int LevelListDrawable::getIntrinsicWidth() const {
    if (mCurrentDrawable) {
        return mCurrentDrawable->getIntrinsicWidth();
    }
    return -1;
}

int LevelListDrawable::getIntrinsicHeight() const {
    if (mCurrentDrawable) {
        return mCurrentDrawable->getIntrinsicHeight();
    }
    return -1;
}

bool LevelListDrawable::getPadding(Rect& padding) const {
    if (mCurrentDrawable) {
        return mCurrentDrawable->getPadding(padding);
    }
    padding.setEmpty();
    return false;
}

void LevelListDrawable::setAlpha(int alpha) {
    mAlpha = alpha;
    if (mCurrentDrawable) {
        mCurrentDrawable->setAlpha(alpha);
    }
}

bool LevelListDrawable::isStateful() const {
    if (mCurrentDrawable) {
        return mCurrentDrawable->isStateful();
    }
    return false;
}

void LevelListDrawable::onBoundsChange(const Rect& bounds) {
    if (mCurrentDrawable) {
        mCurrentDrawable->setBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    }
}

bool LevelListDrawable::onStateChange(const std::vector<int>& stateSet) {
    if (mCurrentDrawable && mCurrentDrawable->isStateful()) {
        return mCurrentDrawable->setState(stateSet);
    }
    return false;
}

bool LevelListDrawable::onLevelChange(int level) {
    DrawablePtr oldDrawable = mCurrentDrawable;
    updateCurrentDrawable(level);
    
    if (oldDrawable != mCurrentDrawable) {
        return true;
    }
    
    if (mCurrentDrawable) {
        return mCurrentDrawable->setLevel(level);
    }
    return false;
}

void LevelListDrawable::updateCurrentDrawable(int level) {
    for (const auto& item : mItems) {
        if (level >= item.minLevel && level <= item.maxLevel) {
            if (mCurrentDrawable != item.drawable) {
                mCurrentDrawable = item.drawable;
                // Apply current bounds, state, alpha
                if (mCurrentDrawable) {
                    const Rect& b = getBounds();
                    mCurrentDrawable->setBounds(b.left, b.top, b.right, b.bottom);
                    mCurrentDrawable->setState(getState());
                    mCurrentDrawable->setAlpha(mAlpha);
                }
                invalidateSelf();
            }
            return;
        }
    }
    
    if (mCurrentDrawable) {
        mCurrentDrawable.reset();
        invalidateSelf();
    }
}

} // namespace graphics
} // namespace setu
