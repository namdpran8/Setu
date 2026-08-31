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

#include "LayerDrawable.h"
#include <algorithm>

namespace setu {
namespace graphics {

void LayerDrawable::addLayer(const DrawablePtr& drawable, int insetLeft, int insetTop, int insetRight, int insetBottom) {
    if (drawable) {
        mLayers.push_back({drawable, insetLeft, insetTop, insetRight, insetBottom});
        // Pass current state/level
        drawable->setState(getState());
        drawable->setLevel(getLevel());
        invalidateSelf();
    }
}

void LayerDrawable::draw(Canvas& canvas) {
    for (const auto& layer : mLayers) {
        if (layer.drawable) {
            layer.drawable->draw(canvas);
        }
    }
}

int LayerDrawable::getIntrinsicWidth() const {
    int w = -1;
    for (const auto& layer : mLayers) {
        if (layer.drawable) {
            int childW = layer.drawable->getIntrinsicWidth();
            if (childW >= 0) {
                childW += layer.insetLeft + layer.insetRight;
                w = std::max(w, childW);
            }
        }
    }
    return w;
}

int LayerDrawable::getIntrinsicHeight() const {
    int h = -1;
    for (const auto& layer : mLayers) {
        if (layer.drawable) {
            int childH = layer.drawable->getIntrinsicHeight();
            if (childH >= 0) {
                childH += layer.insetTop + layer.insetBottom;
                h = std::max(h, childH);
            }
        }
    }
    return h;
}

bool LayerDrawable::getPadding(Rect& padding) const {
    // Android's LayerDrawable usually accumulates padding from all layers
    // For simplicity, returning false unless explicitly set (which we don't yet).
    // Or we could sum them up. Let's just return false for now.
    padding.setEmpty();
    return false;
}

void LayerDrawable::setAlpha(int alpha) {
    mAlpha = alpha;
    for (auto& layer : mLayers) {
        if (layer.drawable) {
            layer.drawable->setAlpha(alpha);
        }
    }
}

bool LayerDrawable::isStateful() const {
    for (const auto& layer : mLayers) {
        if (layer.drawable && layer.drawable->isStateful()) return true;
    }
    return false;
}

void LayerDrawable::onBoundsChange(const Rect& bounds) {
    for (auto& layer : mLayers) {
        if (layer.drawable) {
            Rect insetBounds(
                bounds.left + layer.insetLeft,
                bounds.top + layer.insetTop,
                bounds.right - layer.insetRight,
                bounds.bottom - layer.insetBottom
            );
            layer.drawable->setBounds(insetBounds.left, insetBounds.top, insetBounds.right, insetBounds.bottom);
        }
    }
}

bool LayerDrawable::onStateChange(const std::vector<int>& stateSet) {
    bool changed = false;
    for (auto& layer : mLayers) {
        if (layer.drawable && layer.drawable->isStateful()) {
            if (layer.drawable->setState(stateSet)) {
                changed = true;
            }
        }
    }
    return changed;
}

bool LayerDrawable::onLevelChange(int level) {
    bool changed = false;
    for (auto& layer : mLayers) {
        if (layer.drawable) {
            if (layer.drawable->setLevel(level)) {
                changed = true;
            }
        }
    }
    return changed;
}

} // namespace graphics
} // namespace setu
