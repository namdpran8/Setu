#pragma once
#include "TextView.h"
#include "../graphics/Paint.h"

namespace windroid {
namespace widget {

class EditText : public TextView {
public:
    EditText();
    virtual ~EditText() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onDraw(graphics::Canvas& canvas) override;
    bool onTouchEvent(view::MotionEvent& event) override;
    bool onKeyEvent(const view::KeyEvent& event) override;

private:
    graphics::Paint mLinePaint;
    graphics::Paint mBackgroundPaint;
    bool mIsFocused = false;
};

} // namespace widget
} // namespace windroid
