#pragma once
#include "ViewGroup.h"
#include <map>
#include <string>

namespace windroid {
namespace view {

class RelativeLayout : public ViewGroup {
public:
    class LayoutParams : public View::LayoutParams {
    public:
        // Rule indices (using strings or ints representing view IDs)
        // For simplicity, we'll map rule strings (e.g., "layout_below") to target View IDs
        std::map<std::string, int> rules;

        static const std::string ABOVE;
        static const std::string BELOW;
        static const std::string LEFT_OF;
        static const std::string RIGHT_OF;
        static const std::string ALIGN_PARENT_LEFT;
        static const std::string ALIGN_PARENT_TOP;
        static const std::string ALIGN_PARENT_RIGHT;
        static const std::string ALIGN_PARENT_BOTTOM;

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
    };

    RelativeLayout() = default;
    virtual ~RelativeLayout() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;

private:
    std::shared_ptr<View> getViewById(int id);
    void applyRules(std::shared_ptr<View> child, std::shared_ptr<LayoutParams> lp, int parentWidth, int parentHeight);
};

} // namespace view
} // namespace windroid

