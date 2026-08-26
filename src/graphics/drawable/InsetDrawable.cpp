#include "InsetDrawable.h"

namespace setu {
namespace graphics {

InsetDrawable::InsetDrawable(DrawablePtr child, int inset)
    : InsetDrawable(std::move(child), inset, inset, inset, inset) {}

InsetDrawable::InsetDrawable(DrawablePtr child, int insetLeft, int insetTop, int insetRight,
                             int insetBottom)
    : mDrawable(std::move(child)), mInsets(insetLeft, insetTop, insetRight, insetBottom) {
    if (mDrawable) {
        // `this`, not getCallback(): an <inset> is built before its owner exists,
        // so passing the owner's pointer along would install a null one and a
        // stateful child would never manage to repaint itself. RippleDrawable's
        // setContent carries the same note.
        mDrawable->setCallback(this);
    }
}

void InsetDrawable::draw(Canvas& canvas) {
    // No translate: the child was given inset *bounds*, and a drawable paints
    // inside its own bounds rather than at the canvas origin.
    if (mDrawable) mDrawable->draw(canvas);
}

void InsetDrawable::onBoundsChange(const Rect& bounds) {
    if (!mDrawable) return;
    // Not clamped, as in AOSP: insets larger than the bounds produce an empty rect
    // and the child declines to paint, which is the same nothing a real device
    // draws for an over-inset background.
    mDrawable->setBounds(bounds.left + mInsets.left, bounds.top + mInsets.top,
                         bounds.right - mInsets.right, bounds.bottom - mInsets.bottom);
}

bool InsetDrawable::getPadding(Rect& padding) const {
    Rect childPadding;
    const bool childHasPadding = mDrawable && mDrawable->getPadding(childPadding);

    // The insets are part of the owner's content box, not just of the child's
    // geometry: a Button's label sits inset+padding from the view edge, which on a
    // real device is 4dp+8dp horizontally and 6dp+4dp vertically.
    padding.set(childPadding.left + mInsets.left, childPadding.top + mInsets.top,
                childPadding.right + mInsets.right, childPadding.bottom + mInsets.bottom);

    // True when there is anything to apply, from either source. A zero-inset
    // <inset> around a paddingless child has to report false so that the owner
    // leaves whatever padding it already had alone.
    return childHasPadding ||
           (mInsets.left | mInsets.top | mInsets.right | mInsets.bottom) != 0;
}

int InsetDrawable::getIntrinsicWidth() const {
    if (!mDrawable) return -1;
    const int childWidth = mDrawable->getIntrinsicWidth();
    // -1 means "no inherent size", so it has to survive the addition: a solid
    // colour that stretches to any bounds still stretches once wrapped.
    if (childWidth < 0) return -1;
    return childWidth + mInsets.left + mInsets.right;
}

int InsetDrawable::getIntrinsicHeight() const {
    if (!mDrawable) return -1;
    const int childHeight = mDrawable->getIntrinsicHeight();
    if (childHeight < 0) return -1;
    return childHeight + mInsets.top + mInsets.bottom;
}

bool InsetDrawable::isStateful() const {
    return mDrawable && mDrawable->isStateful();
}

bool InsetDrawable::onStateChange(const std::vector<int>& stateSet) {
    // Guarded on isStateful the way AOSP's DrawableWrapper is: pushing state into
    // a child that cannot use it costs a virtual call per press for nothing.
    if (mDrawable && mDrawable->isStateful()) {
        return mDrawable->setState(stateSet);
    }
    return false;
}

bool InsetDrawable::onLevelChange(int level) {
    return mDrawable ? mDrawable->setLevel(level) : false;
}

void InsetDrawable::setAlpha(int alpha) {
    if (mDrawable) mDrawable->setAlpha(alpha);
}

int InsetDrawable::getAlpha() const {
    return mDrawable ? mDrawable->getAlpha() : 255;
}

bool InsetDrawable::setVisible(bool visible, bool restart) {
    const bool changed = Drawable::setVisible(visible, restart);
    if (mDrawable) mDrawable->setVisible(visible, restart);
    return changed;
}

void InsetDrawable::setHotspot(float x, float y) {
    // Unchanged, not offset by the insets: a hotspot is in the owner's coordinate
    // space, and the child's inset bounds are in that same space rather than in a
    // translated one. AOSP's DrawableWrapper forwards it verbatim too.
    if (mDrawable) mDrawable->setHotspot(x, y);
}

void InsetDrawable::setHotspotBounds(int left, int top, int right, int bottom) {
    if (mDrawable) mDrawable->setHotspotBounds(left, top, right, bottom);
}

void InsetDrawable::jumpToCurrentState() {
    if (mDrawable) mDrawable->jumpToCurrentState();
}

void InsetDrawable::invalidateDrawable(Drawable* who) {
    if (who == mDrawable.get()) {
        invalidateSelf();
    }
}

void InsetDrawable::scheduleDrawable(Drawable* who, std::function<void()> what, long long whenMs) {
    if (who != mDrawable.get()) return;
    // Qualified because this class derives from both Drawable and
    // Drawable::Callback, which makes an unqualified Callback reachable by two
    // paths.
    if (Drawable::Callback* callback = getCallback()) {
        callback->scheduleDrawable(this, std::move(what), whenMs);
    }
}

void InsetDrawable::unscheduleDrawable(Drawable* who) {
    if (who != mDrawable.get()) return;
    if (Drawable::Callback* callback = getCallback()) {
        callback->unscheduleDrawable(this);
    }
}

} // namespace graphics
} // namespace setu
