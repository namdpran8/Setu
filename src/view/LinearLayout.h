#pragma once
#include "ViewGroup.h"

namespace setu {
namespace view {

class LinearLayout : public ViewGroup {
public:
    enum class Orientation {
        HORIZONTAL,
        VERTICAL
    };

    class LayoutParams : public View::LayoutParams {
    public:
        float weight = 0.0f;
        int gravity = -1; // -1 means unspecified

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
        LayoutParams(int w, int h, float wght) : View::LayoutParams(w, h), weight(wght) {}
    };

    LinearLayout() = default;
    virtual ~LinearLayout() = default;

    void setOrientation(Orientation orientation) { mOrientation = orientation; }
    Orientation getOrientation() const { return mOrientation; }

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;

private:
    Orientation mOrientation = Orientation::VERTICAL;
};

} // namespace view
} // namespace setu

