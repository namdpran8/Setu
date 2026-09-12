/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#pragma once
#include <vector>
#include <cstdint>
#include "MotionEvent.h"

namespace setu {
namespace view {

class VelocityTracker {
public:
    static VelocityTracker* obtain();
    void recycle();

    void addMovement(const MotionEvent& event);
    void computeCurrentVelocity(int units, float maxVelocity = -1);

    float getXVelocity() const { return mXVelocity; }
    float getYVelocity() const { return mYVelocity; }

    void clear();

private:
    VelocityTracker() = default;
    ~VelocityTracker() = default;

    struct Movement {
        int64_t eventTime;
        float x;
        float y;
    };

    std::vector<Movement> mMovements;
    float mXVelocity = 0;
    float mYVelocity = 0;
};

} // namespace view
} // namespace setu
