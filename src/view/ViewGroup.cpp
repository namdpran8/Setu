#include "ViewGroup.h"
#include <algorithm>
#include "MotionEvent.h"

namespace windroid {
namespace view {

void ViewGroup::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // Base ViewGroup (used as a static container by LayoutInflater)
    // We shouldn't override children's measured dimensions here since
    // LayoutInflater already computed them statically.
    setMeasuredDimension(View::getSize(widthMeasureSpec), View::getSize(heightMeasureSpec));
}

void ViewGroup::onLayout(bool changed, int l, int t, int r, int b) {
    // We shouldn't force children to 0,0 here since LayoutInflater
    // already placed them at specific X,Y coordinates.
}

void ViewGroup::onDraw(graphics::Canvas& canvas) {
    // ViewGroup draws its own background (handled by View base class)
    // and then dispatches draw to children
    dispatchDraw(canvas);
}

void ViewGroup::dispatchDraw(graphics::Canvas& canvas) {
    for (auto& child : mChildren) {
        child->updateRenderNode();
        canvas.drawRenderNode(child->getRenderNode());
    }
}

bool ViewGroup::dispatchTouchEvent(MotionEvent& event) {
    float x = event.getX();
    float y = event.getY();

    // Iterate backwards for Z-ordering (top views first)
    for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it) {
        auto& child = *it;
        if (x >= child->getLeft() && x <= child->getRight() &&
            y >= child->getTop() && y <= child->getBottom()) {
            
            // Transform coordinates to child's local space
            float offsetX = -(float)child->getLeft();
            float offsetY = -(float)child->getTop();
            
            event.offsetLocation(offsetX, offsetY);
            
            if (child->dispatchTouchEvent(event)) {
                // Event handled by child
                event.offsetLocation(-offsetX, -offsetY); // Restore
                return true;
            }
            
            // Restore coordinates if not handled
            event.offsetLocation(-offsetX, -offsetY);
        }
    }

    // If no child handled it, try handling it ourselves
    return View::dispatchTouchEvent(event);
}

void ViewGroup::measureChild(std::shared_ptr<View> child, int parentWidthMeasureSpec, int parentHeightMeasureSpec) {
    // A real implementation would parse LayoutParams.
    // For now, we just pass down exactly or at_most based on the parent.
    child->measure(parentWidthMeasureSpec, parentHeightMeasureSpec);
}

// ----------------------------------------------------------------------------
// LinearLayout implementation
// ----------------------------------------------------------------------------

void LinearLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int totalWidth = 0;
    int totalHeight = 0;

    for (auto& child : mChildren) {
        // Simple unconstrained measure for children
        measureChild(child, View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED),
                            View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED));
        
        if (mOrientation == Orientation::HORIZONTAL) {
            totalWidth += child->getMeasuredWidth();
            totalHeight = std::max(totalHeight, child->getMeasuredHeight());
        } else {
            totalHeight += child->getMeasuredHeight();
            totalWidth = std::max(totalWidth, child->getMeasuredWidth());
        }
    }

    // Resolve with parent constraints
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : totalWidth;
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : totalHeight;

    if (widthMode == MEASURE_SPEC_AT_MOST) measuredWidth = std::min(measuredWidth, widthSize);
    if (heightMode == MEASURE_SPEC_AT_MOST) measuredHeight = std::min(measuredHeight, heightSize);

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void LinearLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int currentX = 0;
    int currentY = 0;

    for (auto& child : mChildren) {
        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        child->layout(currentX, currentY, currentX + cw, currentY + ch);

        if (mOrientation == Orientation::HORIZONTAL) {
            currentX += cw;
        } else {
            currentY += ch;
        }
    }
}

} // namespace view
} // namespace windroid
