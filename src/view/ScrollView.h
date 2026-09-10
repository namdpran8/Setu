/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once
#include "FrameLayout.h"
#include <cmath>

namespace setu {
namespace view {

class ScrollView : public FrameLayout {
public:
    ScrollView() = default;
    virtual ~ScrollView() = default;

    bool onInterceptTouchEvent(MotionEvent& event) override;
    bool onTouchEvent(MotionEvent& event) override;

private:
    float mLastMotionY = 0;
    bool mIsBeingDragged = false;
    int mTouchSlop = 8;
};

} // namespace view
} // namespace setu
