/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once

#include "../view/ViewGroup.h"
#include "../view/VelocityTracker.h"
#include "OverScroller.h"
#include <unordered_map>
#include <vector>

namespace setu {
namespace widget {

class ViewHolder {
public:
    std::shared_ptr<setu::view::View> itemView;
    int viewType = 0;
    int position = -1;
    void* dalvikHolder = nullptr;
    ViewHolder(std::shared_ptr<setu::view::View> view) : itemView(view) {}
    virtual ~ViewHolder() = default;
};

class Adapter {
public:
    virtual ~Adapter() = default;
    virtual std::shared_ptr<ViewHolder> onCreateViewHolder(setu::view::ViewGroup* parent, int viewType) = 0;
    virtual void onBindViewHolder(std::shared_ptr<ViewHolder> holder, int position) = 0;
    virtual int getItemCount() = 0;
    virtual int getItemViewType(int position) { return 0; }
};

class LinearLayoutManager {
public:
    void layoutChildren(class RecyclerView* recycler, Adapter* adapter, int width, int height, int scrollY);
};

class RecycledViewPool {
public:
    void putRecycledView(std::shared_ptr<ViewHolder> scrap);
    std::shared_ptr<ViewHolder> getRecycledView(int viewType);
private:
    std::unordered_map<int, std::vector<std::shared_ptr<ViewHolder>>> mScrap;
};

class RecyclerView : public setu::view::ViewGroup {
public:
    RecyclerView();
    virtual ~RecyclerView();

    void setAdapter(std::shared_ptr<Adapter> adapter) { mAdapter = adapter; requestLayout(); }
    std::shared_ptr<Adapter> getAdapter() const { return mAdapter; }

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;
    void draw(graphics::Canvas& canvas) override;

    bool onInterceptTouchEvent(setu::view::MotionEvent& event) override;
    bool onTouchEvent(setu::view::MotionEvent& event) override;

    void computeScroll();

    std::shared_ptr<RecycledViewPool> getRecycledViewPool() { return mPool; }
    
    // Internal API for LayoutManager
    std::shared_ptr<ViewHolder> getViewForPosition(int position);
    void addViewHolder(std::shared_ptr<ViewHolder> holder);
    void recycleViewHolder(std::shared_ptr<ViewHolder> holder);
    void recycleAllActiveHolders();

    static int getChildMeasureSpec_public(int spec, int padding, int childDimension) {
        return getChildMeasureSpec(spec, padding, childDimension);
    }

private:
    std::shared_ptr<Adapter> mAdapter;
    LinearLayoutManager mLayoutManager;
    std::shared_ptr<RecycledViewPool> mPool;

    std::unordered_map<std::shared_ptr<setu::view::View>, std::shared_ptr<ViewHolder>> mChildToHolder;
    std::vector<std::shared_ptr<ViewHolder>> mActiveHolders;

    float mLastMotionY = 0;
    bool mIsBeingDragged = false;
    setu::view::VelocityTracker* mVelocityTracker = nullptr;
    std::shared_ptr<OverScroller> mScroller;
};

} // namespace widget
} // namespace setu
