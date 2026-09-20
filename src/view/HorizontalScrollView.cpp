/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "HorizontalScrollView.h"
#include <algorithm>

namespace setu {
namespace view {

void HorizontalScrollView::onLayout(bool changed, int l, int t, int r, int b) {
    FrameLayout::onLayout(changed, l, t, r, b);
    scrollTo(mScrollX, mScrollY);
}

int HorizontalScrollView::getScrollRange() const {
    if (mChildren.empty() || !mChildren[0]) return 0;

    auto child = mChildren[0];
    auto lp = child->getLayoutParams();
    int childWidth = child->getWidth();
    if (lp) {
        childWidth += lp->leftMargin + lp->rightMargin;
    }

    int parentSpace = getWidth() - mPaddingLeft - mPaddingRight;
    return std::max(0, childWidth - parentSpace);
}

void HorizontalScrollView::scrollTo(int x, int) {
    View::scrollTo(std::clamp(x, 0, getScrollRange()), 0);
}

bool HorizontalScrollView::onInterceptTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionX = event.getX();
        mIsBeingDragged = false;
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        float x = event.getX();
        float xDiff = std::abs(x - mLastMotionX);
        if (xDiff > mTouchSlop) {
            mIsBeingDragged = true;
            mLastMotionX = x;
        }
    } else if (event.getAction() == MotionEvent::Action::UP || event.getAction() == MotionEvent::Action::CANCEL) {
        mIsBeingDragged = false;
    }
    return mIsBeingDragged;
}

bool HorizontalScrollView::onTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionX = event.getX();
        mIsBeingDragged = true;
        return true;
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        if (!mIsBeingDragged) {
            float x = event.getX();
            float xDiff = std::abs(x - mLastMotionX);
            if (xDiff > mTouchSlop) {
                mIsBeingDragged = true;
                mLastMotionX = x;
            }
        }
        if (mIsBeingDragged) {
            float x = event.getX();
            int deltaX = (int)(mLastMotionX - x);
            mLastMotionX = x;
            
            int newScrollX = std::clamp(mScrollX + deltaX, 0, getScrollRange());
            scrollTo(newScrollX, 0);
            return true;
        }
    } else if (event.getAction() == MotionEvent::Action::UP || event.getAction() == MotionEvent::Action::CANCEL) {
        mIsBeingDragged = false;
        return true;
    }
    return false;
}

} // namespace view
} // namespace setu
