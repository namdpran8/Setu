/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "ScrollView.h"
#include <algorithm>

namespace setu {
namespace view {

void ScrollView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    FrameLayout::onMeasure(widthMeasureSpec, heightMeasureSpec);
    
    if (!mChildren.empty() && mChildren[0] && mChildren[0]->getVisibility() != View::GONE) {
        auto child = mChildren[0];
        auto lp = std::dynamic_pointer_cast<FrameLayout::LayoutParams>(child->getLayoutParams());
        if (!lp) return;
        
        int childWidthMeasureSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
        int childHeightMeasureSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
        
        child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
    }
}

void ScrollView::onLayout(bool changed, int l, int t, int r, int b) {
    FrameLayout::onLayout(changed, l, t, r, b);
    scrollTo(mScrollX, mScrollY);
}

int ScrollView::getScrollRange() const {
    if (mChildren.empty() || !mChildren[0]) return 0;

    auto child = mChildren[0];
    auto lp = child->getLayoutParams();
    int childHeight = child->getHeight();
    if (lp) {
        childHeight += lp->topMargin + lp->bottomMargin;
    }

    int parentSpace = getHeight() - mPaddingTop - mPaddingBottom;
    return std::max(0, childHeight - parentSpace);
}

void ScrollView::scrollTo(int, int y) {
    View::scrollTo(0, std::clamp(y, 0, getScrollRange()));
}

ScrollView::ScrollView() {
    mScroller = std::make_shared<widget::OverScroller>();
}

ScrollView::~ScrollView() {
    if (mVelocityTracker) {
        mVelocityTracker->recycle();
        mVelocityTracker = nullptr;
    }
}

bool ScrollView::onInterceptTouchEvent(MotionEvent& event) {
    float touchSlop = 8.0f * View::getDisplayDensity();
    if (!mVelocityTracker) mVelocityTracker = VelocityTracker::obtain();
    mVelocityTracker->addMovement(event);

    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = !mScroller->isFinished();
        if (mIsBeingDragged) {
            mScroller->abortAnimation();
        }
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        float y = event.getY();
        float yDiff = std::abs(y - mLastMotionY);
        if (yDiff > touchSlop) {
            mIsBeingDragged = true;
            mLastMotionY = y;
        }
    } else if (event.getAction() == MotionEvent::Action::UP || event.getAction() == MotionEvent::Action::CANCEL) {
        mIsBeingDragged = false;
        if (mVelocityTracker) { mVelocityTracker->recycle(); mVelocityTracker = nullptr; }
    }
    return mIsBeingDragged;
}

bool ScrollView::onTouchEvent(MotionEvent& event) {
    float touchSlop = 8.0f * View::getDisplayDensity();
    if (!mVelocityTracker) mVelocityTracker = VelocityTracker::obtain();
    mVelocityTracker->addMovement(event);

    if (event.getAction() == MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = true;
        if (!mScroller->isFinished()) {
            mScroller->abortAnimation();
        }
        return true;
    } else if (event.getAction() == MotionEvent::Action::MOVE) {
        if (!mIsBeingDragged) {
            float y = event.getY();
            float yDiff = std::abs(y - mLastMotionY);
            if (yDiff > touchSlop) {
                mIsBeingDragged = true;
                mLastMotionY = y;
            }
        }
        if (mIsBeingDragged) {
            float y = event.getY();
            int deltaY = (int)(mLastMotionY - y);
            mLastMotionY = y;
            
            int newScrollY = std::clamp(mScrollY + deltaY, 0, getScrollRange());
            scrollTo(0, newScrollY);
            return true;
        }
    } else if (event.getAction() == MotionEvent::Action::UP || event.getAction() == MotionEvent::Action::CANCEL) {
        if (mIsBeingDragged && event.getAction() == MotionEvent::Action::UP) {
            mVelocityTracker->computeCurrentVelocity(1000, 8000.0f);
            int initialVelocity = (int)mVelocityTracker->getYVelocity();
            if (std::abs(initialVelocity) > 50) {
                mScroller->fling(mScrollX, mScrollY, 0, -initialVelocity, 0, 0, 0, getScrollRange());
                invalidate();
            }
        }
        mIsBeingDragged = false;
        if (mVelocityTracker) { mVelocityTracker->recycle(); mVelocityTracker = nullptr; }
        return true;
    }
    return false;
}

void ScrollView::computeScroll() {
    if (mScroller->computeScrollOffset()) {
        scrollTo(mScroller->getCurrX(), mScroller->getCurrY());
        invalidate();
    }
}

void ScrollView::draw(graphics::Canvas& canvas) {
    computeScroll();
    FrameLayout::draw(canvas);
}

} // namespace view
} // namespace setu
