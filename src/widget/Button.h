#pragma once
#include "TextView.h"
#include <functional>
#include <memory>

namespace setu {
namespace widget {

class Button : public TextView {
public:
    Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    Button();
    virtual ~Button() = default;

    // Reports the press through setPressed(), so the background <selector> is what
    // decides how a pressed button looks. Nothing here knows about colours any
    // more, which is why an app's own selector now works as well as the built-in
    // one.
    bool onTouchEvent(view::MotionEvent& event) override;

    std::string getClassName() const override { return "Button"; }
};

} // namespace widget
} // namespace setu
