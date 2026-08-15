#pragma once
#include "View.h"
#include "../graphics/Direct2DCanvas.h"
#include <memory>

namespace setu {
namespace view {

class Choreographer {
public:
    static Choreographer& getInstance() {
        static Choreographer instance;
        return instance;
    }

    void doFrame(std::shared_ptr<View> decorView, graphics::Direct2DCanvas& canvas, int windowWidth, int windowHeight);

private:
    Choreographer() = default;
};

} // namespace view
} // namespace setu
