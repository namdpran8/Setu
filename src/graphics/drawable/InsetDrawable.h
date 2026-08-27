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

namespace setu {
namespace graphics {

// android.graphics.drawable.InsetDrawable: the <inset> element.
//
// Holds one child and draws it inside a smaller rectangle than the one it was
// given, leaving a transparent margin. That is how every stock Material button
// gets the gap between its background and the view's own edges -
// btn_default_mtrl_shape is an <inset> of 4dp horizontally and 6dp vertically
// around a <shape> - and it is why two adjacent buttons on a real device have
// space between them without either layout naming a margin.
//
// This is not the same thing as padding and cannot be faked with it. Padding
// shrinks the *content* box while the background still fills the whole view, so
// a button faked that way still paints flush against the window edge and merely
// crams its label inwards. An inset shrinks the background and leaves the view's
// bounds - and therefore its touch target - exactly where they were.
//
// The AOSP details that matter, all of which live in DrawableWrapper on a real
// device and are folded in here because nothing else needs that base class yet:
//
//   * the child's bounds are the owner's bounds minus the insets, recomputed on
//     every resize (onBoundsChange);
//   * getPadding() reports the child's padding *plus* the insets, which is what
//     makes the whole background - inset and all - define the owner's content
//     box, exactly as View::setBackground expects;
//   * the intrinsic size grows by the insets, and a child with none (-1) still
//     reports none, so wrapping a solid colour does not invent a size for it;
//   * everything an owner pushes in - state, level, alpha, visibility, hotspot -
//     is forwarded, and the child's repaint requests are forwarded back out, so
//     wrapping a <selector> or a <ripple> does not break its reaction to touch.
//
// Deliberately not implemented: AOSP's fractional insets (android:insetLeft as a
// percentage of the bounds), which resolve per-resize rather than once. Nothing
// in the framework's own resources uses them.
//
// There is no setter for the child on purpose: the insets and the child are both
// known by the time either is useful, so a future <inset> branch in
// DrawableInflater can read its attributes, inflate the nested element, and then
// construct one of these.
class InsetDrawable : public Drawable, public Drawable::Callback {
public:
    // The same inset on all four edges, as AOSP's android:inset.
    InsetDrawable(DrawablePtr child, int inset);
    InsetDrawable(DrawablePtr child, int insetLeft, int insetTop, int insetRight, int insetBottom);

    Drawable* getDrawable() const { return mDrawable.get(); }
    const Rect& getInsets() const { return mInsets; }

    void draw(Canvas& canvas) override;

    bool getPadding(Rect& padding) const override;
    bool isStateful() const override;

    // getMinimumWidth/Height are deliberately not overridden: the base class
    // derives them from getIntrinsicWidth/Height, which already account for the
    // insets, and AOSP's DrawableWrapper leaves them alone for the same reason.
    int getIntrinsicWidth() const override;
    int getIntrinsicHeight() const override;

    void setAlpha(int alpha) override;
    int getAlpha() const override;

    bool setVisible(bool visible, bool restart) override;
    void setHotspot(float x, float y) override;
    void setHotspotBounds(int left, int top, int right, int bottom) override;
    void jumpToCurrentState() override;

    // graphics::Drawable::Callback. The child repainting itself is this drawable
    // repainting itself; without this a wrapped <ripple> would animate against a
    // callback that reaches nothing.
    void invalidateDrawable(Drawable* who) override;
    void scheduleDrawable(Drawable* who, std::function<void()> what, long long whenMs) override;
    void unscheduleDrawable(Drawable* who) override;

protected:
    void onBoundsChange(const Rect& bounds) override;
    bool onStateChange(const std::vector<int>& stateSet) override;
    bool onLevelChange(int level) override;

private:
    // Owned outright, the same trade DrawableContainer makes: one inflation per
    // View instead of a shared ConstantState plus mutate().
    DrawablePtr mDrawable;

    // Positive amounts to pull each edge inward, not a rectangle in any space.
    Rect mInsets;
};

} // namespace graphics
} // namespace setu
