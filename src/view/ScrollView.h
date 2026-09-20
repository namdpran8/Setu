/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once
#include "FrameLayout.h"
#include <cmath>
#include <memory>
#include "VelocityTracker.h"
#include "../widget/OverScroller.h"

namespace setu {
namespace view {

class ScrollView : public FrameLayout {
public:
    ScrollView();
    virtual ~ScrollView();

    bool onInterceptTouchEvent(MotionEvent& event) override;
    bool onTouchEvent(MotionEvent& event) override;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;
    void scrollTo(int x, int y) override;
    void computeScroll();
    void draw(graphics::Canvas& canvas) override;

private:
    int getScrollRange() const;
    float mLastMotionY = 0;
    bool mIsBeingDragged = false;
    VelocityTracker* mVelocityTracker = nullptr;
    std::shared_ptr<widget::OverScroller> mScroller;
};

} // namespace view
} // namespace setu
