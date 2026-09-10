/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "HorizontalScrollView.h"
#include <algorithm>

namespace setu {
namespace view {

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
            
            // Calculate max scroll
            int childWidth = 0;
            if (!mChildren.empty() && mChildren[0]) {
                childWidth = mChildren[0]->getRight();
            }
            int maxScrollX = std::max(0, childWidth - (getRight() - getLeft()));
            
            int newScrollX = std::clamp(mScrollX + deltaX, 0, maxScrollX);
            scrollTo(newScrollX, mScrollY);
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
