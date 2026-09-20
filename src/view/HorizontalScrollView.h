/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once
#include "FrameLayout.h"
#include <cmath>

namespace setu {
namespace view {

class HorizontalScrollView : public FrameLayout {
public:
    HorizontalScrollView() = default;
    virtual ~HorizontalScrollView() = default;

    bool onInterceptTouchEvent(MotionEvent& event) override;
    bool onTouchEvent(MotionEvent& event) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;
    void scrollTo(int x, int y) override;

private:
    int getScrollRange() const;
    float mLastMotionX = 0;
    bool mIsBeingDragged = false;
    int mTouchSlop = 8;
};

} // namespace view
} // namespace setu
