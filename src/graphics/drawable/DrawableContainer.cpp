#include "DrawableContainer.h"

namespace setu {
namespace graphics {

void DrawableContainer::draw(Canvas& canvas) {
    if (mCurrDrawable) {
        mCurrDrawable->draw(canvas);
    }
}

int DrawableContainer::addChild(DrawablePtr child) {
    if (!child) return -1;

    const int pos = (int)mChildren.size();
    // Invisible until selected, and marked so *before* the callback is installed
    // so the change does not bounce out to a container that is not showing it.
    child->setVisible(false, true);
    child->setCallback(this);
    mChildren.push_back(std::move(child));
    invalidateCache();
    return pos;
}

Drawable* DrawableContainer::getChild(int index) const {
    if (index < 0 || index >= (int)mChildren.size()) return nullptr;
    return mChildren[index].get();
}

bool DrawableContainer::selectDrawable(int index) {
    if (index == mCurIndex) return false;

    if (mCurrDrawable) {
        mCurrDrawable->setVisible(false, false);
    }

    if (index >= 0 && index < (int)mChildren.size()) {
        Drawable* child = mChildren[index].get();
        mCurrDrawable = child;
        mCurIndex = index;
        if (child) {
            initializeDrawableForDisplay(child);
        }
    } else {
        mCurrDrawable = nullptr;
        mCurIndex = -1;
    }

    invalidateSelf();
    return true;
}

void DrawableContainer::initializeDrawableForDisplay(Drawable* child) {
    mBlockInvalidate = true;

    if (mHasAlpha) {
        child->setAlpha(mAlpha);
    }
    child->setVisible(isVisible(), true);
    child->setState(getState());
    child->setLevel(getLevel());
    child->setBounds(getBounds());
    if (mHasHotspotBounds) {
        child->setHotspotBounds(mHotspotBounds.left, mHotspotBounds.top,
                                mHotspotBounds.right, mHotspotBounds.bottom);
    }

    mBlockInvalidate = false;
}

void DrawableContainer::onBoundsChange(const Rect& bounds) {
    if (mCurrDrawable) {
        mCurrDrawable->setBounds(bounds);
    }
}

bool DrawableContainer::onStateChange(const std::vector<int>& stateSet) {
    // The current child, not all of them: an unselected child is caught up when
    // it becomes current. StateListDrawable calls this first and then re-selects,
    // so the child that ends up showing always has the new state either way.
    if (mCurrDrawable) {
        return mCurrDrawable->setState(stateSet);
    }
    return false;
}

bool DrawableContainer::onLevelChange(int level) {
    if (mCurrDrawable) {
        return mCurrDrawable->setLevel(level);
    }
    return false;
}

bool DrawableContainer::isStateful() const {
    if (mCheckedStateful) return mStateful;

    mStateful = false;
    for (const auto& child : mChildren) {
        if (child && child->isStateful()) {
            mStateful = true;
            break;
        }
    }
    mCheckedStateful = true;
    return mStateful;
}

const Rect* DrawableContainer::getConstantPadding() const {
    if (mVariablePadding) return nullptr;

    if (mCheckedPadding) {
        return mHasConstantPadding ? &mConstantPadding : nullptr;
    }

    mHasConstantPadding = false;
    mConstantPadding.setEmpty();

    Rect childPadding;
    for (const auto& child : mChildren) {
        if (!child || !child->getPadding(childPadding)) continue;
        mHasConstantPadding = true;
        // Per-edge maximum, so no state can pull the content inwards or outwards
        // relative to another.
        if (childPadding.left > mConstantPadding.left) mConstantPadding.left = childPadding.left;
        if (childPadding.top > mConstantPadding.top) mConstantPadding.top = childPadding.top;
        if (childPadding.right > mConstantPadding.right) mConstantPadding.right = childPadding.right;
        if (childPadding.bottom > mConstantPadding.bottom) mConstantPadding.bottom = childPadding.bottom;
    }

    mCheckedPadding = true;
    return mHasConstantPadding ? &mConstantPadding : nullptr;
}

bool DrawableContainer::getPadding(Rect& padding) const {
    const Rect* constant = getConstantPadding();
    if (constant) {
        padding = *constant;
        // AOSP reports "has padding" only when at least one edge is non-zero,
        // even though it just computed a rect either way.
        return (padding.left | padding.top | padding.right | padding.bottom) != 0;
    }
    if (mCurrDrawable) {
        return mCurrDrawable->getPadding(padding);
    }
    return Drawable::getPadding(padding);
}

void DrawableContainer::computeConstantSize() const {
    mCheckedConstantSize = true;

    mConstantWidth = -1;
    mConstantHeight = -1;
    mConstantMinimumWidth = 0;
    mConstantMinimumHeight = 0;

    for (const auto& child : mChildren) {
        if (!child) continue;
        int size = child->getIntrinsicWidth();
        if (size > mConstantWidth) mConstantWidth = size;
        size = child->getIntrinsicHeight();
        if (size > mConstantHeight) mConstantHeight = size;
        size = child->getMinimumWidth();
        if (size > mConstantMinimumWidth) mConstantMinimumWidth = size;
        size = child->getMinimumHeight();
        if (size > mConstantMinimumHeight) mConstantMinimumHeight = size;
    }
}

int DrawableContainer::getIntrinsicWidth() const {
    if (mConstantSize) {
        if (!mCheckedConstantSize) computeConstantSize();
        return mConstantWidth;
    }
    return mCurrDrawable ? mCurrDrawable->getIntrinsicWidth() : -1;
}

int DrawableContainer::getIntrinsicHeight() const {
    if (mConstantSize) {
        if (!mCheckedConstantSize) computeConstantSize();
        return mConstantHeight;
    }
    return mCurrDrawable ? mCurrDrawable->getIntrinsicHeight() : -1;
}

int DrawableContainer::getMinimumWidth() const {
    if (mConstantSize) {
        if (!mCheckedConstantSize) computeConstantSize();
        return mConstantMinimumWidth;
    }
    return mCurrDrawable ? mCurrDrawable->getMinimumWidth() : 0;
}

int DrawableContainer::getMinimumHeight() const {
    if (mConstantSize) {
        if (!mCheckedConstantSize) computeConstantSize();
        return mConstantMinimumHeight;
    }
    return mCurrDrawable ? mCurrDrawable->getMinimumHeight() : 0;
}

void DrawableContainer::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mHasAlpha && mAlpha == alpha) return;
    mHasAlpha = true;
    mAlpha = alpha;
    if (mCurrDrawable) {
        mCurrDrawable->setAlpha(alpha);
    }
}

bool DrawableContainer::setVisible(bool visible, bool restart) {
    const bool changed = Drawable::setVisible(visible, restart);
    if (mCurrDrawable) {
        mCurrDrawable->setVisible(visible, restart);
    }
    return changed;
}

void DrawableContainer::setHotspot(float x, float y) {
    if (mCurrDrawable) {
        mCurrDrawable->setHotspot(x, y);
    }
}

void DrawableContainer::setHotspotBounds(int left, int top, int right, int bottom) {
    // Kept even when nothing is selected yet, so a child selected later still
    // learns where the touch was. Phase 5's ripple is the reason this matters.
    mHotspotBounds.set(left, top, right, bottom);
    mHasHotspotBounds = true;
    if (mCurrDrawable) {
        mCurrDrawable->setHotspotBounds(left, top, right, bottom);
    }
}

void DrawableContainer::jumpToCurrentState() {
    if (mCurrDrawable) {
        mCurrDrawable->jumpToCurrentState();
        if (mHasAlpha) {
            mCurrDrawable->setAlpha(mAlpha);
        }
    }
}

void DrawableContainer::invalidateDrawable(Drawable* who) {
    if (mBlockInvalidate) return;

    // A child's appearance changing can change what this container reports, so
    // the derived answers have to be re-derived.
    invalidateCache();

    if (who == mCurrDrawable) {
        invalidateSelf();
    }
}

void DrawableContainer::scheduleDrawable(Drawable* who, std::function<void()> what,
                                         long long whenMs) {
    if (who != mCurrDrawable) return;
    // Qualified: this class derives from both Drawable and Drawable::Callback, so an
    // unqualified Callback is reachable by two paths.
    if (Drawable::Callback* callback = getCallback()) {
        callback->scheduleDrawable(this, std::move(what), whenMs);
    }
}

void DrawableContainer::unscheduleDrawable(Drawable* who) {
    if (who != mCurrDrawable) return;
    if (Drawable::Callback* callback = getCallback()) {
        callback->unscheduleDrawable(this);
    }
}

void DrawableContainer::setConstantSize(bool constant) {
    mConstantSize = constant;
    mCheckedConstantSize = false;
}

void DrawableContainer::setVariablePadding(bool variable) {
    mVariablePadding = variable;
    mCheckedPadding = false;
}

void DrawableContainer::invalidateCache() {
    mCheckedPadding = false;
    mCheckedConstantSize = false;
    mCheckedStateful = false;
}

} // namespace graphics
} // namespace setu
