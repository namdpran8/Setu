#pragma once
#include "TextView.h"
#include "../graphics/Paint.h"
#include "../graphics/drawable/GradientDrawable.h"

#include <memory>

namespace setu {
namespace widget {

class EditText : public TextView {
public:
    EditText(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    EditText();
    virtual ~EditText() = default;

    void onDraw(graphics::Canvas& canvas) override;
    bool onTouchEvent(view::MotionEvent& event) override;
    bool onKeyEvent(const view::KeyEvent& event) override;

    std::string getClassName() const override { return "EditText"; }

private:
    // Builds the stand-in field background and installs it. Kept as a helper
    // because both constructors need it and neither can call the other.
    void installDefaultBackground(uint32_t color);

    graphics::Paint mLinePaint;
    // Held so the colour can be changed later without rebuilding the drawable,
    // and so a layout's android:background can be told apart from this default.
    std::shared_ptr<graphics::GradientDrawable> mDefaultBackground;
};

} // namespace widget
} // namespace setu
