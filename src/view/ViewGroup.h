#pragma once
#include "View.h"
#include <vector>
#include <memory>

namespace windroid {
namespace view {

class ViewGroup : public View {
public:
    ViewGroup() = default;
    virtual ~ViewGroup() = default;

    void addView(std::shared_ptr<View> child) {
        if (child) {
            child->setParent(this);
            mChildren.push_back(child);
        }
    }

    void removeView(std::shared_ptr<View> child) {
        auto it = std::find(mChildren.begin(), mChildren.end(), child);
        if (it != mChildren.end()) {
            (*it)->setParent(nullptr);
            mChildren.erase(it);
        }
    }

    size_t getChildCount() const {
        return mChildren.size();
    }

    std::shared_ptr<View> getChildAt(size_t index) const {
        if (index < mChildren.size()) {
            return mChildren[index];
        }
        return nullptr;
    }

    // Measure pass override to measure children (subclasses must implement specific logic)
    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;

    // Layout pass override to layout children (subclasses must implement specific logic)
    void onLayout(bool changed, int l, int t, int r, int b) override;

    // Draw pass override to dispatch drawing to children
    void onDraw(graphics::Canvas& canvas) override;
    
    // Virtual dispatch draw for children
    virtual void dispatchDraw(graphics::Canvas& canvas);

    // Hit testing and event routing
    bool dispatchTouchEvent(MotionEvent& event) override;

protected:
    void measureChild(std::shared_ptr<View> child, int parentWidthMeasureSpec, int parentHeightMeasureSpec);
    
    std::vector<std::shared_ptr<View>> mChildren;
};

} // namespace view
} // namespace windroid
