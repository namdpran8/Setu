#pragma once

namespace windroid {
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
} // namespace windroid
