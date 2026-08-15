#pragma once
#include "View.h"
#include <vector>
#include <memory>

namespace android { class ResXMLParser; }
namespace setu {
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
    
    std::shared_ptr<View> findViewById(int targetId) override;

    // Layout pass override to layout children (subclasses must implement specific logic)
    void onLayout(bool changed, int l, int t, int r, int b) override;

    // Draw pass override to dispatch drawing to children
    void onDraw(graphics::Canvas& canvas) override;
    
    // Virtual dispatch draw for children
    virtual void dispatchDraw(graphics::Canvas& canvas);

    // Hit testing and event routing
    bool dispatchTouchEvent(MotionEvent& event) override;
    bool dispatchKeyEvent(const KeyEvent& event) override;

    virtual std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser);
    static void parseBaseLayoutParams(std::shared_ptr<View::LayoutParams> lp, android::ResXMLParser* parser);

protected:
    void measureChild(std::shared_ptr<View> child, int parentWidthMeasureSpec, int parentHeightMeasureSpec);
    void measureChildWithMargins(std::shared_ptr<View> child, 
        int parentWidthMeasureSpec, int widthUsed,
        int parentHeightMeasureSpec, int heightUsed);
    static int getChildMeasureSpec(int spec, int padding, int childDimension);
    
    std::vector<std::shared_ptr<View>> mChildren;

    virtual void dump(int depth = 0) override;
    virtual std::string getClassName() const override { return "ViewGroup"; }
};

} // namespace view
} // namespace setu

