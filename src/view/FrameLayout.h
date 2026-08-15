#pragma once
#include "ViewGroup.h"

namespace setu {
namespace view {

class FrameLayout : public ViewGroup {
public:
    class LayoutParams : public View::LayoutParams {
    public:
        int gravity = -1; // -1 means top|left usually

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
    };

    FrameLayout() = default;
    virtual ~FrameLayout() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;
};

} // namespace view
} // namespace setu

