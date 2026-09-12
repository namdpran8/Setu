/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "RecyclerView.h"
#include <cmath>
#include <algorithm>

namespace setu {
namespace widget {

void RecycledViewPool::putRecycledView(std::shared_ptr<ViewHolder> scrap) {
    mScrap[scrap->viewType].push_back(scrap);
}

std::shared_ptr<ViewHolder> RecycledViewPool::getRecycledView(int viewType) {
    auto& pool = mScrap[viewType];
    if (!pool.empty()) {
        auto holder = pool.back();
        pool.pop_back();
        return holder;
    }
    return nullptr;
}

void LinearLayoutManager::layoutChildren(RecyclerView* recycler, Adapter* adapter, int width, int height, int scrollY) {
    if (!adapter) return;
    int itemCount = adapter->getItemCount();
    if (itemCount == 0) return;

    // Cache to assume 150px item height
    int estimatedHeight = 150 * setu::view::View::getDisplayDensity();
    int currentY = 0;
    
    // Find first visible item
    int firstVisible = std::max(0, scrollY / estimatedHeight - 1);
    currentY = firstVisible * estimatedHeight;

    // Collect current children and recycle them
    std::vector<std::shared_ptr<setu::view::View>> childrenToRemove;
    for (size_t i = 0; i < recycler->getChildCount(); ++i) {
        childrenToRemove.push_back(recycler->getChildAt(i));
    }
    for (auto child : childrenToRemove) {
        recycler->removeView(child);
    }
    
    // We also need to recycle the ViewHolders
    recycler->recycleAllActiveHolders();

    for (int i = firstVisible; i < itemCount; ++i) {
        if (currentY > scrollY + height + estimatedHeight) {
            break; // Past bottom buffer
        }

        auto holder = recycler->getViewForPosition(i);
        auto child = holder->itemView;
        recycler->addView(child);
        recycler->addViewHolder(holder);

        int childWidthSpec = recycler->getChildMeasureSpec_public(
            setu::view::View::makeMeasureSpec(width, setu::view::View::MEASURE_SPEC_EXACTLY), 0, setu::view::View::MATCH_PARENT);
        int childHeightSpec = setu::view::View::makeMeasureSpec(0, setu::view::View::MEASURE_SPEC_UNSPECIFIED);
        
        child->measure(childWidthSpec, childHeightSpec);
        int measuredHeight = child->getMeasuredHeight();
        
        child->layout(0, currentY, width, currentY + measuredHeight);
        currentY += measuredHeight;
    }
    
    // Max scroll logic
    int totalHeight = itemCount * estimatedHeight;
    int maxScroll = std::max(0, totalHeight - height);
    if (recycler->getScrollY() > maxScroll) {
        recycler->scrollTo(0, maxScroll);
    }
}

RecyclerView::RecyclerView() {
    mScroller = std::make_shared<OverScroller>();
    mPool = std::make_shared<RecycledViewPool>();
}

RecyclerView::~RecyclerView() {
    if (mVelocityTracker) {
        mVelocityTracker->recycle();
        mVelocityTracker = nullptr;
    }
}

void RecyclerView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int width = setu::view::View::getSize(widthMeasureSpec);
    int height = setu::view::View::getSize(heightMeasureSpec);
    setMeasuredDimension(width, height);
}

void RecyclerView::onLayout(bool changed, int l, int t, int r, int b) {
    if (!mAdapter) return;
    mLayoutManager.layoutChildren(this, mAdapter.get(), r - l, b - t, mScrollY);
}

bool RecyclerView::onInterceptTouchEvent(setu::view::MotionEvent& event) {
    float touchSlop = 8.0f * setu::view::View::getDisplayDensity();
    if (!mVelocityTracker) mVelocityTracker = setu::view::VelocityTracker::obtain();
    mVelocityTracker->addMovement(event);

    if (event.getAction() == setu::view::MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = !mScroller->isFinished();
        if (mIsBeingDragged) mScroller->abortAnimation();
    } else if (event.getAction() == setu::view::MotionEvent::Action::MOVE) {
        float y = event.getY();
        if (std::abs(y - mLastMotionY) > touchSlop) {
            mIsBeingDragged = true;
            mLastMotionY = y;
        }
    } else if (event.getAction() == setu::view::MotionEvent::Action::UP || event.getAction() == setu::view::MotionEvent::Action::CANCEL) {
        mIsBeingDragged = false;
        if (mVelocityTracker) { mVelocityTracker->recycle(); mVelocityTracker = nullptr; }
    }
    return mIsBeingDragged;
}

bool RecyclerView::onTouchEvent(setu::view::MotionEvent& event) {
    float touchSlop = 8.0f * setu::view::View::getDisplayDensity();
    if (!mVelocityTracker) mVelocityTracker = setu::view::VelocityTracker::obtain();
    mVelocityTracker->addMovement(event);

    if (event.getAction() == setu::view::MotionEvent::Action::DOWN) {
        mLastMotionY = event.getY();
        mIsBeingDragged = true;
        if (!mScroller->isFinished()) mScroller->abortAnimation();
        return true;
    } else if (event.getAction() == setu::view::MotionEvent::Action::MOVE) {
        if (!mIsBeingDragged) {
            float y = event.getY();
            if (std::abs(y - mLastMotionY) > touchSlop) {
                mIsBeingDragged = true;
                mLastMotionY = y;
            }
        }
        if (mIsBeingDragged) {
            float y = event.getY();
            int deltaY = (int)(mLastMotionY - y);
            mLastMotionY = y;
            
            int estimatedItemHeight = 150 * setu::view::View::getDisplayDensity();
            int itemCount = mAdapter ? mAdapter->getItemCount() : 0;
            int maxScrollY = std::max(0, itemCount * estimatedItemHeight - (getBottom() - getTop()));
            int newScrollY = std::clamp(mScrollY + deltaY, 0, maxScrollY);
            scrollTo(mScrollX, newScrollY);
            requestLayout();
            return true;
        }
    } else if (event.getAction() == setu::view::MotionEvent::Action::UP || event.getAction() == setu::view::MotionEvent::Action::CANCEL) {
        if (mIsBeingDragged && event.getAction() == setu::view::MotionEvent::Action::UP) {
            mVelocityTracker->computeCurrentVelocity(1000, 8000.0f);
            int initialVelocity = (int)mVelocityTracker->getYVelocity();
            if (std::abs(initialVelocity) > 50) {
                int estimatedItemHeight = 150 * setu::view::View::getDisplayDensity();
                int itemCount = mAdapter ? mAdapter->getItemCount() : 0;
                int maxScrollY = std::max(0, itemCount * estimatedItemHeight - (getBottom() - getTop()));
                mScroller->fling(mScrollX, mScrollY, 0, -initialVelocity, 0, 0, 0, maxScrollY);
                invalidate();
            }
        }
        mIsBeingDragged = false;
        if (mVelocityTracker) { mVelocityTracker->recycle(); mVelocityTracker = nullptr; }
        return true;
    }
    return false;
}

void RecyclerView::computeScroll() {
    if (mScroller->computeScrollOffset()) {
        scrollTo(mScroller->getCurrX(), mScroller->getCurrY());
        requestLayout();
        invalidate();
    }
}

void RecyclerView::draw(graphics::Canvas& canvas) {
    computeScroll();
    ViewGroup::draw(canvas);
}

std::shared_ptr<ViewHolder> RecyclerView::getViewForPosition(int position) {
    if (!mAdapter) return nullptr;
    int viewType = mAdapter->getItemViewType(position);
    
    auto holder = mPool->getRecycledView(viewType);
    if (!holder) {
        holder = mAdapter->onCreateViewHolder(this, viewType);
        holder->viewType = viewType;
    }
    mAdapter->onBindViewHolder(holder, position);
    holder->position = position;
    return holder;
}

void RecyclerView::addViewHolder(std::shared_ptr<ViewHolder> holder) {
    if (holder && holder->itemView) {
        mChildToHolder[holder->itemView] = holder;
        mActiveHolders.push_back(holder);
    }
}

void RecyclerView::recycleViewHolder(std::shared_ptr<ViewHolder> holder) {
    if (holder) {
        mPool->putRecycledView(holder);
        if (holder->itemView) {
            mChildToHolder.erase(holder->itemView);
        }
        auto it = std::find(mActiveHolders.begin(), mActiveHolders.end(), holder);
        if (it != mActiveHolders.end()) {
            mActiveHolders.erase(it);
        }
    }
}

void RecyclerView::recycleAllActiveHolders() {
    for (auto holder : mActiveHolders) {
        mPool->putRecycledView(holder);
    }
    mChildToHolder.clear();
    mActiveHolders.clear();
}

} // namespace widget
} // namespace setu
