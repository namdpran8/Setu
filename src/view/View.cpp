#include "View.h"
#include "ViewGroup.h"
#include <algorithm>
#include "../graphics/RecordingCanvas.h"
#include "MotionEvent.h"
#include "../utils/Logger.h"

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

void View::setLayoutParams(std::shared_ptr<LayoutParams> params) {
    mLayoutParams = params;
    requestLayout();
}

void View::requestLayout() {
    mIsLayoutRequested = true;
    if (mParent) {
        mParent->requestLayout();
    }
}

bool View::dispatchTouchEvent(MotionEvent& event) {
    return onTouchEvent(event);
}

bool View::onTouchEvent(MotionEvent& event) {
    if (event.getAction() == MotionEvent::Action::DOWN) {
        if (mOnClickListener) {
            performClick();
            return true;
        }
    }
    return false;
}

bool View::dispatchKeyEvent(const KeyEvent& event) {
    return onKeyEvent(event);
}

bool View::onKeyEvent(const KeyEvent& event) {
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


void View::dump(int depth) {
    std::string indent(depth * 2, ' ');
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s[%s id=%d] bounds=(%d,%d)-(%d,%d) w=%d h=%d",
             indent.c_str(),
             getClassName().c_str(),
             mId,
             mLeft, mTop, mRight, mBottom,
             mRight - mLeft, mBottom - mTop);
    Logger::i("ViewDump", std::string(buffer));
}

} // namespace view
} // namespace windroid

