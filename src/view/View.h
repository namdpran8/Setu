#pragma once
#include <string>
#include <memory>
#include <functional>
#include "../graphics/Canvas.h"
#include "../graphics/RenderNode.h"

namespace windroid {
namespace view {

class ViewGroup;

class View {
public:
    static const int MATCH_PARENT = -1;
    static const int WRAP_CONTENT = -2;

    class LayoutParams {
    public:
        int width;
        int height;
        
        // Margins
        int leftMargin = 0;
        int topMargin = 0;
        int rightMargin = 0;
        int bottomMargin = 0;

        LayoutParams(int w, int h) : width(w), height(h) {}
        virtual ~LayoutParams() = default;
    };

    View();
    virtual ~View() = default;

    int getId() const { return mId; }
    void setId(int id) { mId = id; }

    std::shared_ptr<LayoutParams> getLayoutParams() const { return mLayoutParams; }
    void setLayoutParams(std::shared_ptr<LayoutParams> params) { mLayoutParams = params; }

    // Layout dimensions and positioning (relative to parent)
    int getLeft() const { return mLeft; }
    int getTop() const { return mTop; }
    int getRight() const { return mRight; }
    int getBottom() const { return mBottom; }
    int getWidth() const { return mRight - mLeft; }
    int getHeight() const { return mBottom - mTop; }

    void setLeft(int left) { mLeft = left; }
    void setTop(int top) { mTop = top; }
    void setRight(int right) { mRight = right; }
    void setBottom(int bottom) { mBottom = bottom; }

    // Measured dimensions (what the view wants to be)
    int getMeasuredWidth() const { return mMeasuredWidth; }
    int getMeasuredHeight() const { return mMeasuredHeight; }

    // Parent
    ViewGroup* getParent() const { return mParent; }
    void setParent(ViewGroup* parent) { mParent = parent; }

    // Android Measure/Layout/Draw passes
    virtual void measure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void layout(int l, int t, int r, int b);
    virtual void draw(graphics::Canvas& canvas);

    // To be overridden by subclasses
    virtual void onMeasure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void onLayout(bool changed, int l, int t, int r, int b);
    virtual void onDraw(graphics::Canvas& canvas);

    // Event handling
    virtual bool dispatchTouchEvent(class MotionEvent& event);
    virtual bool onTouchEvent(class MotionEvent& event);

    void setOnClickListener(std::function<void()> listener) { mOnClickListener = listener; }
    virtual void performClick() { if (mOnClickListener) mOnClickListener(); }

    // MeasureSpec constants
    static const int MEASURE_SPEC_UNSPECIFIED = 0 << 30;
    static const int MEASURE_SPEC_EXACTLY = 1 << 30;
    static const int MEASURE_SPEC_AT_MOST = 2 << 30;
    static const int MEASURE_SPEC_MODE_MASK = 3 << 30;

    static int makeMeasureSpec(int size, int mode) {
        return (size & ~MEASURE_SPEC_MODE_MASK) | (mode & MEASURE_SPEC_MODE_MASK);
    }
    static int getMode(int measureSpec) {
        return (measureSpec & MEASURE_SPEC_MODE_MASK);
    }
    static int getSize(int measureSpec) {
        return (measureSpec & ~MEASURE_SPEC_MODE_MASK);
    }

    void setMeasuredDimension(int measuredWidth, int measuredHeight);

    // RenderNode integration
    graphics::RenderNode* getRenderNode() const { return mRenderNode.get(); }
    void updateRenderNode();

protected:
    int mId = 0;
    int mLeft = 0;
    int mTop = 0;
    int mRight = 0;
    int mBottom = 0;

    int mMeasuredWidth = 0;
    int mMeasuredHeight = 0;

    ViewGroup* mParent = nullptr;

    std::unique_ptr<graphics::RenderNode> mRenderNode;
    std::function<void()> mOnClickListener;
    std::shared_ptr<LayoutParams> mLayoutParams;
};

} // namespace view
} // namespace windroid
