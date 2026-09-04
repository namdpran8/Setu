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

#include <functional>
#include <memory>
#include <vector>

#include "../Rect.h"
#include "../ColorFilter.h"

namespace setu {
namespace graphics {

class Canvas;

// Windroid's equivalent of android.graphics.drawable.Drawable.
//
// Everything a real APK uses to paint chrome - solid colours, <shape>s, state
// selectors, ripples, nine-patches - is one of these. Before it existed a View's
// background was a single uint32_t, which meant a <shape> or <selector> in a
// layout resolved to "not a color, ignoring" and simply did not draw.
//
// Three pieces of AOSP behaviour matter and are easy to get wrong:
//   * bounds are pushed in by the owner, never queried from it;
//   * appearance changes are reported *up* through Callback, so a drawable never
//     needs to know what a View is;
//   * getPadding() lets a drawable claim content insets (a <shape> with
//     <padding>, or a nine-patch's content box), which the owner applies.
class Drawable {
public:
    // android.graphics.drawable.Drawable.Callback.
    class Callback {
    public:
        virtual ~Callback() = default;

        // "I look different now; redraw me."
        virtual void invalidateDrawable(Drawable* who) = 0;

        // Animation hooks. Used from Phase 5 (ripple) onwards; a host that
        // cannot post delayed work can leave these alone.
        //
        // `whenMs` is an absolute uptimeMillis() deadline, not a delay - the same
        // as AOSP's Handler.postAtTime. An animation that computes its next
        // deadline from the previous one therefore keeps its cadence instead of
        // drifting a little later on every frame.
        virtual void scheduleDrawable(Drawable* who, std::function<void()> what, long long whenMs) {}
        virtual void unscheduleDrawable(Drawable* who) {}
    };

    Drawable() = default;
    virtual ~Drawable() = default;

    Drawable(const Drawable&) = delete;
    Drawable& operator=(const Drawable&) = delete;

    // Paints inside getBounds(). Implementations must not assume the canvas is
    // translated for them.
    virtual void draw(Canvas& canvas) = 0;

    void setBounds(int left, int top, int right, int bottom);
    void setBounds(const Rect& bounds) {
        setBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    }
    const Rect& getBounds() const { return mBounds; }

    void setCallback(Callback* callback) { mCallback = callback; }
    Callback* getCallback() const { return mCallback; }

    // Tells the owner to redraw. Silently does nothing when unowned, which is
    // the normal case for a drawable still being inflated.
    void invalidateSelf();

    // Asks the owner to run `what` at absolute time `whenMs`, and to forget any
    // work already queued for this drawable. Both are silent no-ops when unowned
    // or when the host cannot post delayed work - which is the degradation an
    // animating drawable has to survive, by looking correct at rest.
    void scheduleSelf(std::function<void()> what, long long whenMs);
    void unscheduleSelf();

    // 0..255, multiplied into whatever colours the drawable itself defines.
    virtual void setAlpha(int alpha) {}
    virtual int getAlpha() const { return 255; }

    virtual void setColorFilter(ColorFilterPtr colorFilter) { mColorFilter = colorFilter; }
    virtual void setTint(uint32_t tintColor) { mTint = tintColor; mHasTint = true; }
    virtual void setTintMode(BlendMode tintMode) { mTintMode = tintMode; mHasTintMode = true; }

    ColorFilterPtr getActiveColorFilter() const {
        if (mColorFilter) return mColorFilter;
        if (mHasTint) {
            return std::make_shared<PorterDuffColorFilter>(mTint, mHasTintMode ? mTintMode : BlendMode::SRC_IN);
        }
        return nullptr;
    }

    // True if the drawable's appearance depends on the owner's state (pressed,
    // enabled, ...). Owners only bother pushing state into stateful drawables.
    virtual bool isStateful() const { return false; }

    // Returns true if the appearance changed as a result.
    bool setState(const std::vector<int>& stateSet);
    const std::vector<int>& getState() const { return mStateSet; }

    // 0..10000, as used by progress bars and <clip>/<scale> drawables.
    bool setLevel(int level);
    int getLevel() const { return mLevel; }

    // -1 means "no inherent size": a solid colour stretches to any bounds, a
    // bitmap does not.
    virtual int getIntrinsicWidth() const { return -1; }
    virtual int getIntrinsicHeight() const { return -1; }

    virtual int getMinimumWidth() const {
        const int w = getIntrinsicWidth();
        return w > 0 ? w : 0;
    }
    virtual int getMinimumHeight() const {
        const int h = getIntrinsicHeight();
        return h > 0 ? h : 0;
    }

    // Content insets the owner should turn into padding. Returns false and zeroes
    // `padding` when there are none, matching AOSP so callers can skip the work.
    virtual bool getPadding(Rect& padding) const;

    virtual bool isVisible() const { return mVisible; }
    virtual bool setVisible(bool visible, bool restart);

    // Animatable interface
    virtual void start() {}
    virtual void stop() {}
    virtual bool isRunning() const { return false; }


    // Ripples and other touch-anchored effects need to know where the finger
    // went down. A no-op for everything else.
    virtual void setHotspot(float x, float y) {}
    virtual void setHotspotBounds(int left, int top, int right, int bottom) {}

    // Skip any in-flight transition and land on the current state immediately.
    virtual void jumpToCurrentState() {}

protected:
    // Called after the bounds actually change. Subclasses rebuild cached
    // geometry here rather than during draw().
    virtual void onBoundsChange(const Rect& bounds) {}

    // Return true if the new state changes the drawable's appearance.
    virtual bool onStateChange(const std::vector<int>& stateSet) { return false; }
    virtual bool onLevelChange(int level) { return false; }

    ColorFilterPtr mColorFilter;
    uint32_t mTint = 0;
    bool mHasTint = false;
    BlendMode mTintMode = BlendMode::SRC_IN;
    bool mHasTintMode = false;

private:
    Rect mBounds;
    std::vector<int> mStateSet;
    Callback* mCallback = nullptr;
    int mLevel = 0;
    bool mVisible = true;
};

using DrawablePtr = std::shared_ptr<Drawable>;

} // namespace graphics
} // namespace setu
