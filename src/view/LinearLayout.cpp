#include "LinearLayout.h"
#include <algorithm>

namespace windroid {
namespace view {

void LinearLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int totalWidth = 0;
    int totalHeight = 0;
    float totalWeight = 0.0f;
    
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Pass 1: Measure unweighted children
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = 0;
        int childHeightSpec = 0;
        
        // Handle horizontal
        if (mOrientation == Orientation::HORIZONTAL) {
            if (lp->width == 0 && lp->weight > 0) {
                totalWeight += lp->weight;
                continue; // Defer measurement
            } else if (lp->width == View::MATCH_PARENT) {
                childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_EXACTLY);
            } else if (lp->width == View::WRAP_CONTENT) {
                childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
            }
            
            if (lp->height == View::MATCH_PARENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_EXACTLY);
            } else if (lp->height == View::WRAP_CONTENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
            }
        } 
        // Handle vertical
        else {
            if (lp->height == 0 && lp->weight > 0) {
                totalWeight += lp->weight;
                continue; // Defer measurement
            } else if (lp->height == View::MATCH_PARENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_EXACTLY);
            } else if (lp->height == View::WRAP_CONTENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
            }
            
            if (lp->width == View::MATCH_PARENT) {
                childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_EXACTLY);
            } else if (lp->width == View::WRAP_CONTENT) {
                childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
            }
        }

        child->measure(childWidthSpec, childHeightSpec);
        
        if (mOrientation == Orientation::HORIZONTAL) {
            totalWidth += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
            totalHeight = std::max(totalHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
        } else {
            totalHeight += child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
            totalWidth = std::max(totalWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        }
    }

    // Pass 2: Measure weighted children
    if (totalWeight > 0.0f) {
        int remainingWidth = std::max(0, widthSize - totalWidth);
        int remainingHeight = std::max(0, heightSize - totalHeight);

        for (auto& child : mChildren) {
            auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
            if (!lp || lp->weight == 0.0f) continue;
            
            if (mOrientation == Orientation::HORIZONTAL && lp->width == 0) {
                int share = (int)(remainingWidth * (lp->weight / totalWeight));
                int childWidthSpec = View::makeMeasureSpec(share, View::MEASURE_SPEC_EXACTLY);
                int childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
                if (lp->height == View::MATCH_PARENT) childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_EXACTLY);
                else if (lp->height != View::WRAP_CONTENT) childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
                
                child->measure(childWidthSpec, childHeightSpec);
                totalWidth += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
                totalHeight = std::max(totalHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
            } else if (mOrientation == Orientation::VERTICAL && lp->height == 0) {
                int share = (int)(remainingHeight * (lp->weight / totalWeight));
                int childHeightSpec = View::makeMeasureSpec(share, View::MEASURE_SPEC_EXACTLY);
                int childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_AT_MOST);
                if (lp->width == View::MATCH_PARENT) childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_EXACTLY);
                else if (lp->width != View::WRAP_CONTENT) childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
                
                child->measure(childWidthSpec, childHeightSpec);
                totalHeight += child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
                totalWidth = std::max(totalWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
            }
        }
    }

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
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        if (mOrientation == Orientation::HORIZONTAL) {
            currentX += lp->leftMargin;
            // Handle vertical gravity for child if specified (simplified to top/center/bottom)
            int childTop = currentY + lp->topMargin;
            child->layout(currentX, childTop, currentX + cw, childTop + ch);
            currentX += cw + lp->rightMargin;
        } else {
            currentY += lp->topMargin;
            // Handle horizontal gravity for child if specified
            int childLeft = currentX + lp->leftMargin;
            child->layout(childLeft, currentY, childLeft + cw, currentY + ch);
            currentY += ch + lp->bottomMargin;
        }
    }
}

} // namespace view
} // namespace windroid
