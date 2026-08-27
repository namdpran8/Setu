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

#include <cstdint>
#include <memory>
#include <vector>

#include "Drawable.h"
#include "../ColorStateList.h"

namespace setu {
namespace graphics {

// android.graphics.drawable.RippleDrawable: the touch feedback every Material
// widget is built on.
//
//   <ripple xmlns:android="..." android:color="?colorControlHighlight">
//     <item android:drawable="@drawable/button_shape"/>
//     <item android:id="@android:id/mask" android:drawable="@android:color/white"/>
//   </ripple>
//
// Before this, a <ripple> background inflated to nothing at all, which is why a
// stock AppCompat button rendered as a hole. It is also the first drawable in the
// runtime that *animates*, so it is where the frame clock arrives: see
// Drawable::scheduleSelf and View::runScheduledWork.
//
// Four deliberate departures from AOSP, each forced by something below this class
// rather than chosen:
//
//   * No animators. Every value here is a pure function of uptimeMillis(), so a
//     ripple is fully described by where and when it started. A late or dropped
//     frame changes nothing, and there is no per-frame mutable state to get out of
//     step with the clock.
//   * No LayerDrawable underneath. AOSP's RippleDrawable extends it to hold an
//     arbitrary layer stack; drawable containers are not on the roadmap yet, so
//     this keeps one content layer and one mask layer, which is what essentially
//     every real <ripple> resource actually contains.
//   * No mask shader. AOSP builds an ALPHA_8 bitmap from the mask or content and
//     draws the ripple through it; Paint has no shader and no colour filter, so
//     this takes AOSP's other branch - MASK_NONE, where clipping to the bounds
//     does the containing. The visible cost is corners: a ripple on a rounded
//     button squares off at the bounds instead of following the radius.
//   * Solid style only. STYLE_PATTERNED (Android 12's "sparkle") needs a
//     RuntimeShader, so this is the STYLE_SOLID path that every version before it
//     used and that every version since still falls back to.
//
// Like DrawableContainer, this is its content layer's Callback rather than passing
// its owner's along: the owner is installed after inflation, so a forwarded
// pointer would be null at the moment the child needs one.
class RippleDrawable : public Drawable, public Drawable::Callback {
public:
    // android:radius, when absent: expand to the bounds' own diagonal.
    static constexpr int RADIUS_AUTO = -1;

    // The colour used when android:color resolves to nothing for the current
    // state. AOSP passes Color.BLACK here, not 0 - unlike <shape>'s solid and
    // stroke, which fall back to transparent. A ripple with an unmatched state
    // still ripples, faintly.
    static constexpr uint32_t DEFAULT_COLOR = 0xFF000000;

    // Timing, in milliseconds, from RippleForeground and RippleBackground.
    static constexpr long long RIPPLE_ENTER_DURATION = 225;   // radius and origin
    static constexpr long long OPACITY_ENTER_DURATION = 75;   // fade in
    static constexpr long long OPACITY_EXIT_DURATION = 150;   // fade out
    // A ripple released early still holds full opacity until this long after it
    // started, so a quick tap is not a flicker. AOSP spells it as
    // OPACITY_ENTER_DURATION + 150.
    static constexpr long long OPACITY_HOLD_DURATION = OPACITY_ENTER_DURATION + 150;
    static constexpr long long BACKGROUND_OPACITY_DURATION = 80;

    // The fraction of the larger bound a ripple starts at, rather than starting
    // from nothing. AOSP: "take 60% of the maximum of the width and height, then
    // divide in half to get the radius."
    static constexpr float START_RADIUS_FRACTION = 0.3f;

    // Hover and focus background opacities. Focus wins when both are set.
    static constexpr float FOCUSED_OPACITY = 0.6f;
    static constexpr float HOVERED_OPACITY = 0.2f;

    // AOSP's cap, with its own comment: "this should never happen unless the user
    // is tapping like a maniac".
    static constexpr size_t MAX_RIPPLES = 10;

    // Requested gap between animation frames. A request, not a promise - the host
    // clock is a WM_TIMER and coarser than this. Nothing depends on it being
    // honoured, because every value is read from the clock rather than stepped.
    static constexpr long long FRAME_INTERVAL_MS = 16;

    // `color` is android:color, which <ripple> requires. Null is accepted rather
    // than asserted - AOSP throws on the missing attribute at inflate time, which
    // is DrawableInflater's job, and a drawable built in code should still be
    // usable as an invisible one.
    explicit RippleDrawable(ColorStateListPtr color);

    void draw(Canvas& canvas) override;

    // The non-mask child: what the ripple plays over. Optional; a ripple with no
    // content is the `?selectableItemBackground` case, which draws touch feedback
    // and nothing else.
    void setContent(DrawablePtr content);
    Drawable* getContent() const { return mContent.get(); }

    // The @android:id/mask child. Held, never drawn - AOSP does not draw it
    // either, it converts it to a shader. Without shaders its only remaining
    // effect is on isBounded(), which is still worth honouring: a mask-only
    // <ripple> is a bounded one.
    void setMask(DrawablePtr mask);
    Drawable* getMask() const { return mMask.get(); }

    // android:radius. RADIUS_AUTO means the bounds' diagonal half-length.
    void setMaxRadius(int radius);
    int getMaxRadius() const { return mMaxRadius; }

    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

    // Unconditionally true, as in AOSP: a ripple's whole purpose is to respond to
    // state, so there is no configuration in which its owner should skip telling
    // it about one.
    bool isStateful() const override { return true; }

    // True when the ripple is contained by something - a content layer or a mask.
    // A bare <ripple> is unbounded and, on a real device, projects onto whatever
    // is behind it.
    bool isBounded() const { return mContent != nullptr || mMask != nullptr; }

    bool getPadding(Rect& padding) const override;
    int getIntrinsicWidth() const override;
    int getIntrinsicHeight() const override;

    bool setVisible(bool visible, bool restart) override;

    void setHotspot(float x, float y) override;
    void setHotspotBounds(int left, int top, int right, int bottom) override;
    const Rect& getHotspotBounds() const { return mHotspotBounds; }

    void jumpToCurrentState() override;

    // graphics::Drawable::Callback. The content layer reports through here, and
    // this passes it on under its own identity - an owner only knows about the
    // drawable it installed.
    void invalidateDrawable(Drawable* who) override;
    void scheduleDrawable(Drawable* who, std::function<void()> what, long long whenMs) override;
    void unscheduleDrawable(Drawable* who) override;

protected:
    void onBoundsChange(const Rect& bounds) override;
    bool onStateChange(const std::vector<int>& stateSet) override;

private:
    // One ripple, which in AOSP is a RippleForeground plus four ObjectAnimators.
    // Here it is four numbers: everything else is derived, so this is a value with
    // no update step and nothing to keep in sync.
    struct Ripple {
        // Where the finger went down, in the drawable's own coordinates. Stored
        // raw and clamped on read, so a bounds change re-clamps for free.
        float startX = 0.0f;
        float startY = 0.0f;
        long long enterMs = 0;
        // -1 while the finger is still down. Set once, on release.
        long long exitMs = -1;

        bool hasExited() const { return exitMs >= 0; }
    };

    // The moment a released ripple starts fading. Never earlier than
    // enterMs + OPACITY_HOLD_DURATION, which is what holds a quick tap visible.
    static long long fadeStartMs(const Ripple& ripple);
    // The moment nothing about this ripple changes again.
    static long long endMs(const Ripple& ripple);

    // 0..1 progress along the radius and origin curves.
    static float enterTween(const Ripple& ripple, long long now);
    static float opacity(const Ripple& ripple, long long now);

    float startRadius() const;
    float targetRadius() const;

    // The touch point pulled inside the circle the ripple will grow into, so a
    // ripple started near a corner cannot expand past its own target radius on the
    // far side. AOSP's clampStartingPosition().
    void clampedStart(const Ripple& ripple, float& outX, float& outY) const;

    void drawRipple(Canvas& canvas, const Ripple& ripple, long long now, uint32_t color);
    void drawBackground(Canvas& canvas, long long now, uint32_t color);

    // Half-alpha, as AOSP does, so the background and a ripple over it sum to the
    // authored colour rather than doubling it.
    uint32_t resolveRippleColor() const;

    bool setRippleActive(bool active);
    bool setBackgroundActive(bool hovered, bool focused, bool pressed);

    bool tryRippleEnter();
    bool tryRippleExit();
    void clearHotspots();

    // Drops ripples that have finished fading. Returns true if any went.
    bool pruneRipples(long long now);

    float backgroundOpacity(long long now) const;
    bool isAnimating(long long now) const;

    // Starts the frame pump if anything is animating and it is not already
    // running. Idempotent, which is what lets every state change just call it.
    void startAnimation();
    void onAnimationFrame();

    ColorStateListPtr mColor;
    DrawablePtr mContent;
    DrawablePtr mMask;

    int mMaxRadius = RADIUS_AUTO;
    int mAlpha = 255;

    // Defaults to getBounds(); only diverges once setHotspotBounds() is called.
    Rect mHotspotBounds;
    bool mOverrideHotspotBounds = false;

    // The last hotspot reported, and whether one has been. Without it a ripple
    // starts from the centre - which is what a keyboard-triggered press should do,
    // and what AOSP falls back to as well.
    float mPendingX = 0.0f;
    float mPendingY = 0.0f;
    bool mHasPending = false;

    // The ripple under the finger, if any, followed by those still fading out.
    std::unique_ptr<Ripple> mRipple;
    std::vector<Ripple> mExitingRipples;
    bool mRippleActive = false;

    // The hover/focus glow: AOSP's RippleBackground, which is a single opacity
    // tween and does not need a class of its own here.
    bool mBackgroundExists = false;
    bool mBackgroundFocused = false;
    bool mBackgroundHovered = false;
    float mBackgroundOpacityFrom = 0.0f;
    float mBackgroundOpacityTo = 0.0f;
    long long mBackgroundStartMs = 0;

    // Frame-pump state. mNextFrameMs is carried forward rather than recomputed
    // from now(), so the cadence does not drift later on every frame.
    bool mAnimationRunning = false;
    long long mNextFrameMs = 0;
};

} // namespace graphics
} // namespace setu
