#pragma once
#include "TextView.h"
#include <functional>

namespace windroid {
namespace widget {

class Button : public TextView {
public:
    Button();
    virtual ~Button() = default;

    void setOnClickListener(std::function<void()> listener);

    // Call this when click hit-testing matches this view bounds
    void performClick();

    void onDraw(graphics::Canvas& canvas) override;
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    
    bool onTouchEvent(view::MotionEvent& event) override;

private:
    std::function<void()> mOnClickListener;
    graphics::Paint mBackgroundPaint;
};

} // namespace widget
} // namespace windroid
