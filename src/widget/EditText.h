#pragma once
#include "TextView.h"
#include "../graphics/Paint.h"

namespace windroid {
namespace widget {

class EditText : public TextView {
public:
    EditText(ResourceManager* resManager, Theme* theme, const struct AxmlNode* node, uint32_t defStyleAttr, uint32_t defStyleRes);
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
