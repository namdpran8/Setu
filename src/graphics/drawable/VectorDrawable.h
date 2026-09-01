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
#include <memory>
#include <string>

// We hold SkPath directly because this is a Skia-only drawable for now.
#include "include/core/SkPath.h"

class SkCanvas;

namespace setu {
namespace graphics {

// Structure for a single <path> inside a <vector>
struct VPath : public std::enable_shared_from_this<VPath> {
    std::string name;
    SkPath path;
    uint32_t fillColor = 0;
    float fillAlpha = 1.0f;
    uint32_t strokeColor = 0;
    float strokeWidth = 0.0f;
    float strokeAlpha = 1.0f;
    float trimPathStart = 0.0f;
    float trimPathEnd = 1.0f;
    float trimPathOffset = 0.0f;
    
    enum class LineCap { BUTT, ROUND, SQUARE };
    LineCap strokeLineCap = LineCap::BUTT;
    
    enum class LineJoin { MITER, ROUND, BEVEL };
    LineJoin strokeLineJoin = LineJoin::MITER;
    
    bool fillEvenOdd = false;

    bool hasFill() const { return (fillColor >> 24) != 0 && fillAlpha > 0.0f; }
    bool hasStroke() const { return (strokeColor >> 24) != 0 && strokeWidth > 0.0f && strokeAlpha > 0.0f; }
};

// Structure for a <group> inside a <vector>
struct VGroup : public std::enable_shared_from_this<VGroup> {
    std::string name;
    float rotation = 0.0f;
    float pivotX = 0.0f;
    float pivotY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float translateX = 0.0f;
    float translateY = 0.0f;

    // Children can be groups or paths
    std::vector<std::shared_ptr<VGroup>> groups;
    std::vector<std::shared_ptr<VPath>> paths;
    
    std::shared_ptr<void> getTargetByName(const std::string& targetName) {
        if (!name.empty() && name == targetName) {
            return shared_from_this();
        }
        for (auto& p : paths) {
            if (!p->name.empty() && p->name == targetName) {
                return p;
            }
        }
        for (auto& g : groups) {
            auto res = g->getTargetByName(targetName);
            if (res) return res;
        }
        return nullptr;
    }
};

class VectorDrawable : public Drawable {
public:
    VectorDrawable() {
        mRootGroup = std::make_shared<VGroup>();
    }

    void draw(Canvas& canvas) override;
    
    int getIntrinsicWidth() const override { return mIntrinsicWidth; }
    int getIntrinsicHeight() const override { return mIntrinsicHeight; }
    
    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }
    
    void setViewportSize(float width, float height) {
        mViewportWidth = width;
        mViewportHeight = height;
    }
    
    void setIntrinsicSize(int width, int height) {
        mIntrinsicWidth = width;
        mIntrinsicHeight = height;
    }

    std::shared_ptr<VGroup> getRootGroup() { return mRootGroup; }
    
    std::shared_ptr<void> getTargetByName(const std::string& targetName) {
        if (!mRootGroup) return nullptr;
        return mRootGroup->getTargetByName(targetName);
    }

    void invalidate() {
        // If animated, we need to mark dirty
        invalidateSelf();
    }

protected:
    void onBoundsChange(const Rect& bounds) override;

private:
    void drawGroup(SkCanvas* canvas, const VGroup& group);
    void drawPath(SkCanvas* canvas, const VPath& vpath);

    std::shared_ptr<VGroup> mRootGroup;
    
    float mViewportWidth = 0.0f;
    float mViewportHeight = 0.0f;
    
    int mIntrinsicWidth = -1;
    int mIntrinsicHeight = -1;
    
    int mAlpha = 255;
    
    // Cached scale mapping viewport -> bounds
    float mScaleX = 1.0f;
    float mScaleY = 1.0f;
    float mTranslateX = 0.0f;
    float mTranslateY = 0.0f;
};

} // namespace graphics
} // namespace setu
