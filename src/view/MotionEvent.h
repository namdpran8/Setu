/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace setu {
namespace view {

class MotionEvent {
public:
    enum class Action {
        DOWN,
        UP,
        MOVE,
        CANCEL
    };

    MotionEvent(Action action, float x, float y) : mAction(action), mX(x), mY(y) {}

    Action getAction() const { return mAction; }
    float getX() const { return mX; }
    float getY() const { return mY; }

    // To properly route events recursively, we need to adjust coordinates 
    // to be relative to the local bounds of the view.
    void offsetLocation(float deltaX, float deltaY) {
        mX += deltaX;
        mY += deltaY;
    }

private:
    Action mAction;
    float mX;
    float mY;
};

} // namespace view
} // namespace setu
