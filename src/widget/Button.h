#pragma once
#include "TextView.h"
#include <functional>

namespace windroid {
namespace widget {

class Button : public TextView {
public:
    Button();
    virtual ~Button() = default;

    void onDraw(graphics::Canvas& canvas) override;
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    
    bool onTouchEvent(view::MotionEvent& event) override;

private:
    graphics::Paint mBackgroundPaint;
};

} // namespace widget
} // namespace windroid
