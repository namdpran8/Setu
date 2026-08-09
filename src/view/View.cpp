#include "View.h"
#include <algorithm>
#include "../graphics/RecordingCanvas.h"
#include "MotionEvent.h"

namespace windroid {
namespace view {

View::View() {}

void View::measure(int widthMeasureSpec, int heightMeasureSpec) {
    onMeasure(widthMeasureSpec, heightMeasureSpec);
}

void View::layout(int l, int t, int r, int b) {
    bool changed = (mLeft != l) || (mTop != t) || (mRight != r) || (mBottom != b);
    if (changed) {
        mLeft = l;
        mTop = t;
        mRight = r;
        mBottom = b;
    }
    onLayout(changed, l, t, r, b);
}

void View::draw(graphics::Canvas& canvas) {
    canvas.save();
    canvas.translate((float)mLeft, (float)mTop);
    
    // We can add canvas.clipRect(0, 0, getWidth(), getHeight()) here later
    onDraw(canvas);
    
    canvas.restore();
}

void View::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // Default implementation: just take the provided size if exactly/at most, 
    // or a default minimum size if unspecified.
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);
    
    int measuredWidth = (widthMode == MEASURE_SPEC_UNSPECIFIED) ? 0 : widthSize;
    int measuredHeight = (heightMode == MEASURE_SPEC_UNSPECIFIED) ? 0 : heightSize;
    
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void View::onLayout(bool changed, int l, int t, int r, int b) {
    // Default implementation does nothing
}

void View::onDraw(graphics::Canvas& canvas) {
    // Default implementation does nothing
}

void View::setMeasuredDimension(int measuredWidth, int measuredHeight) {
    mMeasuredWidth = measuredWidth;
    mMeasuredHeight = measuredHeight;
}

bool View::dispatchTouchEvent(MotionEvent& event) {
    // Basic View just calls onTouchEvent
    return onTouchEvent(event);
}

bool View::onTouchEvent(MotionEvent& event) {
    // Base implementation does nothing
    return false;
}

void View::updateRenderNode() {
    if (!mRenderNode) {
        mRenderNode = std::make_unique<graphics::RenderNode>();
    }
    mRenderNode->clear();

    graphics::RecordingCanvas canvas(mRenderNode.get());
    draw(canvas);
}

} // namespace view
} // namespace windroid
