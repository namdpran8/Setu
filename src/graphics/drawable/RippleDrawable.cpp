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

#include "RippleDrawable.h"

#include <algorithm>
#include <cmath>

#include "StateSet.h"
#include "../Canvas.h"
#include "../Paint.h"
#include "../../utils/Interpolator.h"
#include "../../utils/SystemClock.h"

namespace setu {
namespace graphics {

namespace {

// A circle, via the round-rect primitive. A rounded rectangle whose corner radius
// is exactly half its side length *is* a circle - the four arcs meet tangentially
// with nothing straight left between them - so this needs no new Canvas verb and
// still reaches Direct2D's native arc rendering rather than a Bezier
// approximation. AOSP writes it as Canvas.drawCircle.
void drawCircle(Canvas& canvas, float cx, float cy, float radius, const Paint& paint) {
    canvas.drawRoundRect(cx - radius, cy - radius, cx + radius, cy + radius,
                         radius, radius, paint);
}

// Replaces an ARGB colour's alpha with `alpha` scaled by whatever alpha it
// already carried, which is how an animated opacity composes with an authored
// translucent ripple colour.
uint32_t withOpacity(uint32_t argb, float opacity) {
    const float base = (float)((argb >> 24) & 0xFF);
    int alpha = (int)(base * opacity + 0.5f);
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    return ((uint32_t)alpha << 24) | (argb & 0x00FFFFFF);
}

} // namespace

RippleDrawable::RippleDrawable(ColorStateListPtr color) : mColor(std::move(color)) {}

void RippleDrawable::setContent(DrawablePtr content) {
    if (mContent == content) return;
    if (mContent) mContent->setCallback(nullptr);
    mContent = std::move(content);
    if (mContent) {
        // `this`, not getCallback(): a <ripple> is inflated before its owner exists,
        // so passing the owner's pointer along would install a null one and a
        // stateful content layer would never manage to repaint itself.
        mContent->setCallback(this);
        mContent->setBounds(getBounds());
        if (mContent->isStateful()) mContent->setState(getState());
    }
    invalidateSelf();
}

void RippleDrawable::invalidateDrawable(Drawable* who) {
    if (who == mContent.get()) {
        invalidateSelf();
    }
}

void RippleDrawable::scheduleDrawable(Drawable* who, std::function<void()> what,
                                      long long whenMs) {
    if (who != mContent.get()) return;
    // Qualified because this class derives from both Drawable and Drawable::Callback,
    // which makes an unqualified Callback reachable by two paths.
    //
    // The request goes out under this drawable's identity, so the owner sees work
    // from the drawable it installed. The cost is that unscheduleSelf() cannot tell
    // the content's frames from this drawable's own - the Callback interface carries
    // no `what` to match on, as AOSP's does. It has no consequence yet: nothing that
    // appears as a <ripple> content layer animates.
    if (Drawable::Callback* callback = getCallback()) {
        callback->scheduleDrawable(this, std::move(what), whenMs);
    }
}

void RippleDrawable::unscheduleDrawable(Drawable* who) {
    if (who != mContent.get()) return;
    if (Drawable::Callback* callback = getCallback()) {
        callback->unscheduleDrawable(this);
    }
}

void RippleDrawable::setMask(DrawablePtr mask) {
    if (mMask == mask) return;
    // No callback and no state pushed: the mask is never drawn, so nothing it
    // could report would change a pixel. It is held only so isBounded() can tell a
    // mask-only <ripple> from a bare one.
    mMask = std::move(mask);
    if (mMask) mMask->setBounds(getBounds());
    invalidateSelf();
}

void RippleDrawable::setMaxRadius(int radius) {
    if (mMaxRadius == radius) return;
    mMaxRadius = radius;
    invalidateSelf();
}

void RippleDrawable::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mAlpha == alpha) return;
    mAlpha = alpha;
    if (mContent) mContent->setAlpha(alpha);
    invalidateSelf();
}

bool RippleDrawable::getPadding(Rect& padding) const {
    // The content layer's insets are the ripple's insets. AOSP computes this
    // across the whole layer stack under PADDING_MODE_STACK, which with one
    // content layer is exactly this.
    if (mContent) return mContent->getPadding(padding);
    padding.setEmpty();
    return false;
}

int RippleDrawable::getIntrinsicWidth() const {
    return mContent ? mContent->getIntrinsicWidth() : -1;
}

int RippleDrawable::getIntrinsicHeight() const {
    return mContent ? mContent->getIntrinsicHeight() : -1;
}

bool RippleDrawable::setVisible(bool visible, bool restart) {
    const bool changed = Drawable::setVisible(visible, restart);
    if (mContent) mContent->setVisible(visible, restart);

    if (!visible) {
        // Nothing should be mid-animation behind a hidden view: the frames would be
        // paid for and never seen, and the ripple would reappear part-way through
        // when the view came back.
        clearHotspots();
    } else if (changed) {
        // Came back. A ripple still under the finger is restarted rather than
        // resumed, because there is no meaningful position to resume from - and then
        // everything is snapped to its final state, because animating one in from
        // nothing would replay a ripple the finger already finished making.
        if (mRippleActive) tryRippleEnter();
        jumpToCurrentState();
    }
    return changed;
}

void RippleDrawable::onBoundsChange(const Rect& bounds) {
    if (!mOverrideHotspotBounds) {
        mHotspotBounds = bounds;
    }
    if (mContent) mContent->setBounds(bounds);
    if (mMask) mMask->setBounds(bounds);
    // Radii and clamped start positions are all derived from the bounds at draw
    // time, so a resize needs nothing rebuilt here.
}

void RippleDrawable::setHotspot(float x, float y) {
    // Unconditional, where AOSP guards with `mRipple == null || mBackground ==
    // null`. That guard is nearly always true - most widgets have no hover or focus
    // background - and where it is not, leaving the pending point stale makes the
    // *next* press ripple from the centre instead of from the finger. Nothing here
    // relies on it either way: every press reports a fresh hotspot before it
    // reports the pressed state.
    mPendingX = x;
    mPendingY = y;
    mHasPending = true;

    if (mRipple) {
        // AOSP's RippleForeground.move: the origin jumps to the finger and carries
        // on migrating toward the centre from there, so a ripple tracks a drag.
        mRipple->startX = x;
        mRipple->startY = y;
        invalidateSelf();
    }
}

void RippleDrawable::setHotspotBounds(int left, int top, int right, int bottom) {
    mOverrideHotspotBounds = true;
    mHotspotBounds.set(left, top, right, bottom);
    invalidateSelf();
}

void RippleDrawable::jumpToCurrentState() {
    if (mContent) mContent->jumpToCurrentState();

    // Everything mid-flight lands immediately: exiting ripples are dropped, the
    // background snaps to its target, and a ripple still under the finger is
    // back-dated far enough that it reads as fully entered.
    const bool hadWork = !mExitingRipples.empty() || mRipple != nullptr ||
                         mBackgroundOpacityFrom != mBackgroundOpacityTo;
    mExitingRipples.clear();

    const long long now = uptimeMillis();
    if (mRipple) mRipple->enterMs = now - RIPPLE_ENTER_DURATION;

    mBackgroundOpacityFrom = mBackgroundOpacityTo;
    mBackgroundStartMs = now - BACKGROUND_OPACITY_DURATION;

    mAnimationRunning = false;
    unscheduleSelf();

    if (hadWork) invalidateSelf();
}

bool RippleDrawable::onStateChange(const std::vector<int>& stateSet) {
    bool changed = false;
    if (mContent && mContent->isStateful()) {
        changed |= mContent->setState(stateSet);
    }

    bool enabled = false;
    bool pressed = false;
    bool focused = false;
    bool hovered = false;
    for (int state : stateSet) {
        if (state == StateSet::STATE_ENABLED) enabled = true;
        else if (state == StateSet::STATE_PRESSED) pressed = true;
        else if (state == StateSet::STATE_FOCUSED) focused = true;
        else if (state == StateSet::STATE_HOVERED) hovered = true;
    }

    // A disabled widget does not ripple, however hard it is pressed. AOSP reads
    // state_window_focused here too, but only to steer the patterned style's
    // background, which this does not implement.
    changed |= setRippleActive(enabled && pressed);
    changed |= setBackgroundActive(hovered, focused, pressed);

    return changed;
}

bool RippleDrawable::setRippleActive(bool active) {
    if (mRippleActive == active) return false;
    mRippleActive = active;
    return active ? tryRippleEnter() : tryRippleExit();
}

bool RippleDrawable::setBackgroundActive(bool hovered, bool focused, bool pressed) {
    // AOSP's guard, which looks backwards until you read it twice: a background
    // that is *not* already showing will not start showing while the widget is
    // pressed, but one that is already showing stays. The point is to stop a focus
    // glow popping in underneath a ripple that is already covering it.
    if (!mBackgroundFocused) focused = focused && !pressed;
    if (!mBackgroundHovered) hovered = hovered && !pressed;

    if (!mBackgroundExists) {
        if (!hovered && !focused) return false;   // nothing to show, nothing to build
        mBackgroundExists = true;
    }

    if (mBackgroundHovered == hovered && mBackgroundFocused == focused) return false;
    mBackgroundHovered = hovered;
    mBackgroundFocused = focused;

    const long long now = uptimeMillis();
    // Retarget from wherever the tween had reached, not from the old target, so
    // hover-out-then-in mid-fade does not jump.
    mBackgroundOpacityFrom = backgroundOpacity(now);
    mBackgroundOpacityTo = focused ? FOCUSED_OPACITY : (hovered ? HOVERED_OPACITY : 0.0f);
    mBackgroundStartMs = now;

    startAnimation();
    return true;
}

bool RippleDrawable::tryRippleEnter() {
    if (mExitingRipples.size() >= MAX_RIPPLES) {
        // Would mean a tap rate no hand can produce, or a ripple that never
        // finished exiting. Dropping the new one is better than unbounded growth.
        return false;
    }

    if (!mRipple) {
        mRipple = std::make_unique<Ripple>();
        if (mHasPending) {
            mHasPending = false;
            mRipple->startX = mPendingX;
            mRipple->startY = mPendingY;
        } else {
            // No hotspot reported: a press that did not come from a finger, such as
            // a keyboard or accessibility activation. Ripple from the middle.
            mRipple->startX = mHotspotBounds.exactCenterX();
            mRipple->startY = mHotspotBounds.exactCenterY();
        }
    }

    mRipple->enterMs = uptimeMillis();
    mRipple->exitMs = -1;

    startAnimation();
    return true;
}

bool RippleDrawable::tryRippleExit() {
    if (!mRipple) return false;

    mRipple->exitMs = uptimeMillis();
    mExitingRipples.push_back(*mRipple);
    mRipple.reset();

    startAnimation();
    return true;
}

void RippleDrawable::clearHotspots() {
    const bool hadWork = mRipple != nullptr || !mExitingRipples.empty() ||
                         mBackgroundOpacityFrom != 0.0f || mBackgroundOpacityTo != 0.0f;

    mRipple.reset();
    mRippleActive = false;
    mExitingRipples.clear();

    mBackgroundHovered = false;
    mBackgroundFocused = false;
    mBackgroundOpacityFrom = 0.0f;
    mBackgroundOpacityTo = 0.0f;

    mAnimationRunning = false;
    unscheduleSelf();

    if (hadWork) invalidateSelf();
}

long long RippleDrawable::fadeStartMs(const Ripple& ripple) {
    // AOSP delays the fade by OPACITY_HOLD_DURATION minus however long the ripple
    // has already been alive, which reduces exactly to "no earlier than this far
    // past the start" - and reads far more clearly that way.
    //
    // One degenerate case differs: AOSP guards its subtraction with
    // `timeSinceEnter > 0`, so a press and release inside the same millisecond gets
    // no hold at all and flashes nothing, the opacity ramp having only just begun.
    // Holding it is what OPACITY_HOLD_DURATION is for, so this does not reproduce
    // that.
    return std::max(ripple.exitMs, ripple.enterMs + OPACITY_HOLD_DURATION);
}

long long RippleDrawable::endMs(const Ripple& ripple) {
    if (!ripple.hasExited()) {
        // Still held: the radius and origin settle at RIPPLE_ENTER_DURATION and
        // then nothing moves until release.
        return ripple.enterMs + RIPPLE_ENTER_DURATION;
    }
    return fadeStartMs(ripple) + OPACITY_EXIT_DURATION;
}

float RippleDrawable::enterTween(const Ripple& ripple, long long now) {
    const float fraction = (float)(now - ripple.enterMs) / (float)RIPPLE_ENTER_DURATION;
    return interpolator::fastOutSlowIn(fraction);
}

float RippleDrawable::opacity(const Ripple& ripple, long long now) {
    const float entered =
        interpolator::linear((float)(now - ripple.enterMs) / (float)OPACITY_ENTER_DURATION);
    if (!ripple.hasExited()) return entered;

    const long long fadeStart = fadeStartMs(ripple);
    if (now < fadeStart) return entered;

    // Fading from exactly 1, not from `entered`: fadeStart is never earlier than
    // enterMs + OPACITY_HOLD_DURATION, which is well past the 75ms fade-in, so the
    // ripple is always at full opacity by the time the fade begins. That is AOSP's
    // behaviour too - its exit animator reads its start value when the start delay
    // expires, not when exit() is called.
    return 1.0f - interpolator::linear((float)(now - fadeStart) / (float)OPACITY_EXIT_DURATION);
}

float RippleDrawable::startRadius() const {
    const int larger = std::max(mHotspotBounds.width(), mHotspotBounds.height());
    return (float)larger * START_RADIUS_FRACTION;
}

float RippleDrawable::targetRadius() const {
    if (mMaxRadius >= 0) return (float)mMaxRadius;
    // The half-diagonal, so a ripple started dead centre still reaches every
    // corner at full expansion.
    const float halfWidth = (float)mHotspotBounds.width() * 0.5f;
    const float halfHeight = (float)mHotspotBounds.height() * 0.5f;
    return std::sqrt(halfWidth * halfWidth + halfHeight * halfHeight);
}

void RippleDrawable::clampedStart(const Ripple& ripple, float& outX, float& outY) const {
    const float cX = mHotspotBounds.exactCenterX();
    const float cY = mHotspotBounds.exactCenterY();
    const float dX = ripple.startX - cX;
    const float dY = ripple.startY - cY;
    // The reachable radius: how far the origin may sit from the centre and still
    // have the circle cover the whole target when fully grown.
    const float r = targetRadius() - startRadius();

    if (dX * dX + dY * dY > r * r) {
        const double angle = std::atan2((double)dY, (double)dX);
        outX = cX + (float)(std::cos(angle) * (double)r);
        outY = cY + (float)(std::sin(angle) * (double)r);
    } else {
        outX = ripple.startX;
        outY = ripple.startY;
    }
}

uint32_t RippleDrawable::resolveRippleColor() const {
    // No colour at all means an invisible ripple, not a black one. AOSP cannot
    // reach this case - it throws on a <ripple> with no android:color - so BLACK
    // below is only what an authored ColorStateList falls back to when none of its
    // items match the current state, which is AOSP's own default for that lookup.
    if (!mColor) return 0;

    uint32_t color = mColor->getColorForState(getState(), DEFAULT_COLOR);

    // AOSP: "cut the alpha channel in half so that the ripple and background
    // together yield full alpha." Without it a focused, pressed widget paints the
    // authored colour twice over.
    const uint32_t base = (color >> 24) & 0xFF;
    color = ((base / 2u) << 24) | (color & 0x00FFFFFF);

    if (mAlpha < 255) {
        const uint32_t scaled = (((color >> 24) & 0xFF) * (uint32_t)mAlpha) / 255u;
        color = (scaled << 24) | (color & 0x00FFFFFF);
    }
    return color;
}

float RippleDrawable::backgroundOpacity(long long now) const {
    if (!mBackgroundExists) return 0.0f;
    const float fraction =
        interpolator::linear((float)(now - mBackgroundStartMs) / (float)BACKGROUND_OPACITY_DURATION);
    return interpolator::lerp(mBackgroundOpacityFrom, mBackgroundOpacityTo, fraction);
}

bool RippleDrawable::pruneRipples(long long now) {
    const size_t before = mExitingRipples.size();
    mExitingRipples.erase(
        std::remove_if(mExitingRipples.begin(), mExitingRipples.end(),
                       [now](const Ripple& ripple) { return now >= endMs(ripple); }),
        mExitingRipples.end());
    return mExitingRipples.size() != before;
}

bool RippleDrawable::isAnimating(long long now) const {
    if (mRipple && now < endMs(*mRipple)) return true;
    for (const Ripple& ripple : mExitingRipples) {
        if (now < endMs(ripple)) return true;
    }
    if (mBackgroundExists && mBackgroundOpacityFrom != mBackgroundOpacityTo &&
        now < mBackgroundStartMs + BACKGROUND_OPACITY_DURATION) {
        return true;
    }
    return false;
}

void RippleDrawable::startAnimation() {
    invalidateSelf();

    const long long now = uptimeMillis();
    if (mAnimationRunning || !isAnimating(now)) return;

    mAnimationRunning = true;
    mNextFrameMs = now + FRAME_INTERVAL_MS;
    scheduleSelf([this]() { onAnimationFrame(); }, mNextFrameMs);
}

void RippleDrawable::onAnimationFrame() {
    const long long now = uptimeMillis();
    pruneRipples(now);
    invalidateSelf();

    if (!isAnimating(now)) {
        // One last invalidate above, then stop: the final resting frame still has
        // to be painted, but after that nothing changes and the host clock should
        // not keep running.
        mAnimationRunning = false;
        return;
    }

    mNextFrameMs += FRAME_INTERVAL_MS;
    if (mNextFrameMs <= now) {
        // Fell behind - a slow frame, or the window was not being painted at all.
        // Resync instead of firing a burst of immediate catch-up frames, which
        // would all draw the same thing anyway now that the clock is the source of
        // truth.
        mNextFrameMs = now + FRAME_INTERVAL_MS;
    }
    scheduleSelf([this]() { onAnimationFrame(); }, mNextFrameMs);
}

void RippleDrawable::drawRipple(Canvas& canvas, const Ripple& ripple, long long now,
                                uint32_t color) {
    const float alphaFraction = opacity(ripple, now);
    if (alphaFraction <= 0.0f) return;

    const float tween = enterTween(ripple, now);
    const float radius = interpolator::lerp(startRadius(), targetRadius(), tween);
    if (radius <= 0.0f) return;

    const uint32_t painted = withOpacity(color, alphaFraction);
    if ((painted >> 24) == 0) return;

    float startX = 0.0f;
    float startY = 0.0f;
    clampedStart(ripple, startX, startY);

    // AOSP translates the canvas to the hotspot centre and then lerps the origin
    // from (start - centre) toward (0, 0). Adding the translation back into the
    // lerp collapses to this: the centre of the circle simply travels from the
    // touch point to the centre of the bounds as it expands. Same arithmetic, no
    // save/translate/restore, and no pair of extra commands in the display list.
    const float cx = interpolator::lerp(startX, mHotspotBounds.exactCenterX(), tween);
    const float cy = interpolator::lerp(startY, mHotspotBounds.exactCenterY(), tween);

    Paint paint;
    paint.setStyle(Style::FILL);
    paint.setAntiAlias(true);
    paint.setColor(painted);
    drawCircle(canvas, cx, cy, radius, paint);
}

void RippleDrawable::drawBackground(Canvas& canvas, long long now, uint32_t color) {
    const float opacityFraction = backgroundOpacity(now);
    if (opacityFraction <= 0.0f) return;

    const uint32_t painted = withOpacity(color, opacityFraction);
    if ((painted >> 24) == 0) return;

    Paint paint;
    paint.setStyle(Style::FILL);
    paint.setAntiAlias(true);
    paint.setColor(painted);
    // Fixed at the full target radius and centred: the hover/focus glow does not
    // expand, it only fades.
    drawCircle(canvas, mHotspotBounds.exactCenterX(), mHotspotBounds.exactCenterY(),
               targetRadius(), paint);
}

void RippleDrawable::draw(Canvas& canvas) {
    const long long now = uptimeMillis();
    pruneRipples(now);

    const bool hasRipples = mRipple != nullptr || !mExitingRipples.empty();
    const bool hasBackground = backgroundOpacity(now) > 0.0f;

    // The mask is deliberately absent from this: AOSP does not draw it either.
    if (!mContent && !hasRipples && !hasBackground) return;

    canvas.save();
    if (isBounded()) {
        // This is AOSP's MASK_NONE branch - "clipping handles opaque content" -
        // and it is the whole containment story here, because Paint has no shader
        // to build a real mask from. Rounded content therefore squares off: a
        // ripple on a rounded button fills the corners the button itself leaves
        // empty. Wrong in a pixel diff, but bounded, which the alternative is not.
        const Rect& bounds = getBounds();
        canvas.clipRect((float)bounds.left, (float)bounds.top,
                        (float)bounds.right, (float)bounds.bottom);
    }
    // An unbounded ripple is left unclipped, as in AOSP, where it projects onto
    // whatever is behind. Here View::draw clips to the view anyway, so the two
    // branches currently look identical - but the distinction is the one AOSP
    // makes, and it is what will still be right if that changes.

    if (mContent) mContent->draw(canvas);

    if (hasRipples || hasBackground) {
        const uint32_t color = resolveRippleColor();
        if (hasBackground) drawBackground(canvas, now, color);
        // Exiting ripples first, then the live one on top - a second tap during a
        // fade-out should read as being in front of it.
        for (const Ripple& ripple : mExitingRipples) {
            drawRipple(canvas, ripple, now, color);
        }
        if (mRipple) drawRipple(canvas, *mRipple, now, color);
    }

    canvas.restore();
}

} // namespace graphics
} // namespace setu
