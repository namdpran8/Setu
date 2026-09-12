/*
 * Copyright (c) 2026 Pranshu Namdeo
 */

#include "VelocityTracker.h"
#include <algorithm>
#include "../utils/SystemClock.h"

namespace setu {
namespace view {

VelocityTracker* VelocityTracker::obtain() {
    return new VelocityTracker();
}

void VelocityTracker::recycle() {
    delete this;
}

void VelocityTracker::clear() {
    mMovements.clear();
    mXVelocity = 0;
    mYVelocity = 0;
}

void VelocityTracker::addMovement(const MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        clear();
    }
    mMovements.push_back({setu::uptimeMillis(), event.getX(), event.getY()});
    // Keep only last 10 samples for efficiency and recency
    if (mMovements.size() > 10) {
        mMovements.erase(mMovements.begin());
    }
}

void VelocityTracker::computeCurrentVelocity(int units, float maxVelocity) {
    if (mMovements.size() < 2) {
        mXVelocity = 0;
        mYVelocity = 0;
        return;
    }

    // Use a simple time window of 100ms
    int64_t lastTime = mMovements.back().eventTime;
    
    // Find the oldest movement within the window
    int startIndex = 0;
    for (int i = (int)mMovements.size() - 2; i >= 0; --i) {
        if (lastTime - mMovements[i].eventTime > 100) {
            startIndex = i + 1;
            break;
        }
    }
    
    if (startIndex >= (int)mMovements.size() - 1) {
        startIndex = (int)mMovements.size() - 2;
    }
    
    const auto& oldest = mMovements[startIndex];
    const auto& newest = mMovements.back();
    
    float duration = (float)(newest.eventTime - oldest.eventTime);
    if (duration == 0) duration = 1.0f;
    
    float dx = newest.x - oldest.x;
    float dy = newest.y - oldest.y;
    
    mXVelocity = (dx / duration) * units;
    mYVelocity = (dy / duration) * units;
    
    if (maxVelocity > 0) {
        if (mXVelocity > maxVelocity) mXVelocity = maxVelocity;
        if (mXVelocity < -maxVelocity) mXVelocity = -maxVelocity;
        if (mYVelocity > maxVelocity) mYVelocity = maxVelocity;
        if (mYVelocity < -maxVelocity) mYVelocity = -maxVelocity;
    }
}

} // namespace view
} // namespace setu
