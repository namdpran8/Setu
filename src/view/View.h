#pragma once
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include "../graphics/Canvas.h"
#include "../graphics/RenderNode.h"
#include "MotionEvent.h"
#include "KeyEvent.h"

// Forward declare Context/Theme so we don't need a heavy include

namespace android { class ResXMLParser; }

namespace windroid {
class ResourceManager;
class Theme;
class TypedArray;
namespace view {

class ViewGroup;

class View : public std::enable_shared_from_this<View> {
public:
    static const int MATCH_PARENT = -1;
    static const int WRAP_CONTENT = -2;
    static const int VISIBLE = 0x00000000;
    static const int INVISIBLE = 0x00000004;
    static const int GONE = 0x00000008;

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

    View(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    View();
    virtual ~View() = default;

    int getId() const { return mId; }
    void setId(int id) { mId = id; }
    
    virtual std::shared_ptr<View> findViewById(int targetId);

    std::shared_ptr<LayoutParams> getLayoutParams() const { return mLayoutParams; }
    void setLayoutParams(std::shared_ptr<LayoutParams> params);
    void requestLayout();

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

    int getVisibility() const { return mVisibility; }
    void setVisibility(int visibility) { mVisibility = visibility; }

    // Android Measure/Layout/Draw passes
    virtual void measure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void layout(int l, int t, int r, int b);
    virtual void draw(graphics::Canvas& canvas);

    // To be overridden by subclasses
    virtual void onMeasure(int widthMeasureSpec, int heightMeasureSpec);
    virtual void onLayout(bool changed, int l, int t, int r, int b);
    virtual void onDraw(graphics::Canvas& canvas);

    virtual void dump(int depth = 0);
    virtual std::string getClassName() const { return "View"; }

    // Event handling
    virtual bool dispatchTouchEvent(class MotionEvent& event);
    virtual bool onTouchEvent(class MotionEvent& event);

    virtual bool dispatchKeyEvent(const class KeyEvent& event);
    virtual bool onKeyEvent(const class KeyEvent& event);

    bool isFocused() const { return mIsFocused; }
    virtual void setFocus(bool focus) { mIsFocused = focus; }

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
    bool mIsFocused = false;
    int mVisibility = VISIBLE;
    bool mIsLayoutRequested = false;
};

} // namespace view
} // namespace windroid


