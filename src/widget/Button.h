#pragma once
#include "TextView.h"
#include <functional>

namespace setu {
namespace widget {

class Button : public TextView {
public:
    Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    Button();
    virtual ~Button() = default;

    void onDraw(graphics::Canvas& canvas) override;
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    
    bool onTouchEvent(view::MotionEvent& event) override;

private:
    graphics::Paint mBackgroundPaint;
};

} // namespace widget
} // namespace setu

