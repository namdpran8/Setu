#pragma once

#include <vector>

#include "Drawable.h"

namespace setu {
namespace graphics {

// android.graphics.drawable.DrawableContainer: holds N children and shows exactly
// one of them. StateListDrawable (<selector>) is the subclass that matters for
// real layouts; <level-list> and <transition> are the other two.
//
// Three AOSP behaviours are load-bearing here:
//
//   * everything an owner pushes in - bounds, state, level, alpha, visibility,
//     hotspot - has to be forwarded to whichever child is current, and a child
//     that only becomes current later has to be caught up on all of it
//     (initializeDrawableForDisplay). Miss this and a selector's pressed item
//     paints at 0x0 the first time it is selected.
//   * the container is its children's Callback, so a child repainting itself
//     travels up through the container to the View instead of reaching nothing.
//   * getPadding() reports the *largest* padding across all children by default,
//     not the current child's. That is what keeps a button's text from shifting
//     when it is pressed.
//
// Deliberately not implemented: enterFadeDuration/exitFadeDuration cross-fades.
// They need a frame clock, which Windroid does not have yet, so the swap is
// instant - which is exactly what a selector without those attributes does. The
// AOSP mLastDrawable/animate() machinery exists only to serve them.
class DrawableContainer : public Drawable, public Drawable::Callback {
public:
    DrawableContainer() = default;

    void draw(Canvas& canvas) override;

    bool isStateful() const override;
    bool getPadding(Rect& padding) const override;

    int getIntrinsicWidth() const override;
    int getIntrinsicHeight() const override;
    int getMinimumWidth() const override;
    int getMinimumHeight() const override;

    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

    bool setVisible(bool visible, bool restart) override;
    void setHotspot(float x, float y) override;
    void setHotspotBounds(int left, int top, int right, int bottom) override;
    void jumpToCurrentState() override;

    // graphics::Drawable::Callback. A child asking to be redrawn is this
    // container asking to be redrawn.
    void invalidateDrawable(Drawable* who) override;
    void scheduleDrawable(Drawable* who, std::function<void()> what, long long whenMs) override;
    void unscheduleDrawable(Drawable* who) override;

    // AOSP keeps children in an immutable ConstantState shared between instances
    // and relies on mutate() to split them apart. Here each container owns its
    // children outright: it costs one inflation per View and buys not having to
    // implement mutate(), which is the same trade DrawableInflater already makes
    // by not caching.
    int addChild(DrawablePtr child);
    int getChildCount() const { return (int)mChildren.size(); }
    Drawable* getChild(int index) const;

    // Returns true if the displayed child changed. An index outside the child
    // range selects nothing, which is how a <selector> with no matching item
    // draws nothing rather than drawing the wrong thing.
    bool selectDrawable(int index);
    int getCurrentIndex() const { return mCurIndex; }
    Drawable* getCurrent() const { return mCurrDrawable; }

    // <selector android:constantSize>: report the largest child's intrinsic size
    // instead of the current child's, so the view does not resize on a state
    // change.
    void setConstantSize(bool constant);
    // <selector android:variablePadding>: let padding follow the current child
    // rather than being the maximum across all of them.
    void setVariablePadding(bool variable);

protected:
    void onBoundsChange(const Rect& bounds) override;
    bool onStateChange(const std::vector<int>& stateSet) override;
    bool onLevelChange(int level) override;

private:
    // Brings a newly-current child up to date with everything the owner has
    // pushed into the container so far.
    void initializeDrawableForDisplay(Drawable* child);

    // Largest padding across all children, or nullptr when variablePadding is set
    // or no child claims any.
    const Rect* getConstantPadding() const;
    void computeConstantSize() const;
    void invalidateCache();

    std::vector<DrawablePtr> mChildren;
    // Borrowed from mChildren, which owns it for the container's whole lifetime.
    Drawable* mCurrDrawable = nullptr;
    int mCurIndex = -1;

    int mAlpha = 255;
    // AOSP only forwards alpha once someone has actually set it, so a child keeps
    // whatever alpha it inflated with until then.
    bool mHasAlpha = false;

    bool mConstantSize = false;
    bool mVariablePadding = false;

    Rect mHotspotBounds;
    bool mHasHotspotBounds = false;

    // Catching a child up fires a setter per property, and each one reports a
    // repaint. The container invalidates once for the whole swap instead; AOSP
    // does this by temporarily swapping in a callback that swallows them
    // (BlockInvalidateCallback), which a flag reproduces exactly.
    bool mBlockInvalidate = false;

    // getPadding() and getIntrinsicWidth() are const, but their answers are
    // derived from the children, so the caches are mutable rather than being
    // recomputed on every call.
    mutable bool mCheckedPadding = false;
    mutable bool mHasConstantPadding = false;
    mutable Rect mConstantPadding;

    mutable bool mCheckedConstantSize = false;
    mutable int mConstantWidth = -1;
    mutable int mConstantHeight = -1;
    mutable int mConstantMinimumWidth = 0;
    mutable int mConstantMinimumHeight = 0;

    mutable bool mCheckedStateful = false;
    mutable bool mStateful = false;
};

} // namespace graphics
} // namespace setu
