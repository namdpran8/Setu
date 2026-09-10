/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "ScrollView.h"
#include <algorithm>

namespace setu {
namespace view {

bool ScrollView::onInterceptTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = false;
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        float y = event.getY();
        float yDiff = std::abs(y - mLastMotionY);
        if (yDiff > mTouchSlop) {
            mIsBeingDragged = true;
            mLastMotionY = y;
        }
    } else if (event.getAction() == MotionEvent::Action::UP || event.getAction() == MotionEvent::Action::CANCEL) {
        mIsBeingDragged = false;
    }
    return mIsBeingDragged;
}

bool ScrollView::onTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = true;
        return true;
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        if (!mIsBeingDragged) {
            float y = event.getY();
            float yDiff = std::abs(y - mLastMotionY);
            if (yDiff > mTouchSlop) {
                mIsBeingDragged = true;
                mLastMotionY = y;
            }
        }
        if (mIsBeingDragged) {
            float y = event.getY();
            int deltaY = (int)(mLastMotionY - y);
            mLastMotionY = y;
            
            // Calculate max scroll
            int childHeight = 0;
            if (!mChildren.empty() && mChildren[0]) {
                childHeight = mChildren[0]->getBottom();
            }
            int maxScrollY = std::max(0, childHeight - (getBottom() - getTop()));
            
            int newScrollY = std::clamp(mScrollY + deltaY, 0, maxScrollY);
            scrollTo(mScrollX, newScrollY);
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
