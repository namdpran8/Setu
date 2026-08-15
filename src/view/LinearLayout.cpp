#include "androidfw/Util.h"
#include "LinearLayout.h"
#include <algorithm>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

void LinearLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int totalWidth = mPaddingLeft + mPaddingRight;
    int totalHeight = mPaddingTop + mPaddingBottom;
    float totalWeight = 0.0f;
    
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Pass 1: Measure unweighted children
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = 0;
        int childHeightSpec = 0;
        
        // Handle horizontal
        if (mOrientation == Orientation::HORIZONTAL) {
            if (lp->weight > 0.0f) {
                totalWeight += lp->weight;
                continue; // ALWAYS defer measurement for weighted children
            }
            if (lp->width == View::MATCH_PARENT) {
                int mode = (widthMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                childWidthSpec = View::makeMeasureSpec(widthSize, mode);
            } else if (lp->width == View::WRAP_CONTENT) {
                childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
            }
            
            if (lp->height == View::MATCH_PARENT) {
                int mode = (heightMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                childHeightSpec = View::makeMeasureSpec(heightSize, mode);
            } else if (lp->height == View::WRAP_CONTENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
            }
        } 
        // Handle vertical
        else {
            if (lp->weight > 0.0f) {
                totalWeight += lp->weight;
                continue; // ALWAYS defer measurement for weighted children
            }
            if (lp->height == View::MATCH_PARENT) {
                int mode = (heightMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                childHeightSpec = View::makeMeasureSpec(heightSize, mode);
            } else if (lp->height == View::WRAP_CONTENT) {
                childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
            } else {
                childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
            }
            
            if (lp->width == View::MATCH_PARENT) {
                int mode = (widthMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                childWidthSpec = View::makeMeasureSpec(widthSize, mode);
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
            if (child->getVisibility() == View::GONE) continue;

            auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
            if (!lp || lp->weight == 0.0f) continue;
            
            if (mOrientation == Orientation::HORIZONTAL) {
                int share = (int)(remainingWidth * (lp->weight / totalWeight));
                int childWidthSpec = View::makeMeasureSpec(share, View::MEASURE_SPEC_EXACTLY);
                int childHeightSpec = View::makeMeasureSpec(heightSize, View::MEASURE_SPEC_AT_MOST);
                if (lp->height == View::MATCH_PARENT) {
                    int mode = (heightMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                    childHeightSpec = View::makeMeasureSpec(heightSize, mode);
                } else if (lp->height != View::WRAP_CONTENT) {
                    childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
                }
                child->measure(childWidthSpec, childHeightSpec);
                totalWidth += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
                totalHeight = std::max(totalHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
            } else if (mOrientation == Orientation::VERTICAL) {
                int share = (int)(remainingHeight * (lp->weight / totalWeight));
                int childHeightSpec = View::makeMeasureSpec(share, View::MEASURE_SPEC_EXACTLY);
                int childWidthSpec = View::makeMeasureSpec(widthSize, View::MEASURE_SPEC_AT_MOST);
                if (lp->width == View::MATCH_PARENT) {
                    int mode = (widthMode == View::MEASURE_SPEC_EXACTLY) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST;
                    childWidthSpec = View::makeMeasureSpec(widthSize, mode);
                } else if (lp->width != View::WRAP_CONTENT) {
                    childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
                }
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
    int currentX = mPaddingLeft;
    int currentY = mPaddingTop;

    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        if (mOrientation == Orientation::HORIZONTAL) {
            currentX += lp->leftMargin;
            int childTop = currentY + lp->topMargin;
            int verticalGravity = lp->gravity & 0x70;
            if (verticalGravity == 0x10) { // CENTER_VERTICAL
                childTop = currentY + (b - t - ch - lp->topMargin - lp->bottomMargin - mPaddingTop - mPaddingBottom) / 2 + lp->topMargin;
            } else if (verticalGravity == 0x50) { // BOTTOM
                childTop = currentY + (b - t - mPaddingTop - mPaddingBottom) - ch - lp->bottomMargin;
            }
            child->layout(currentX, childTop, currentX + cw, childTop + ch);
            currentX += cw + lp->rightMargin;
        } else {
            currentY += lp->topMargin;
            int childLeft = currentX + lp->leftMargin;
            int horizontalGravity = lp->gravity & 0x07;
            if (horizontalGravity == 0x01) { // CENTER_HORIZONTAL
                childLeft = currentX + (r - l - cw - lp->leftMargin - lp->rightMargin - mPaddingLeft - mPaddingRight) / 2 + lp->leftMargin;
            } else if (horizontalGravity == 0x05) { // RIGHT
                childLeft = currentX + (r - l - mPaddingLeft - mPaddingRight) - cw - lp->rightMargin;
            }
            child->layout(childLeft, currentY, childLeft + cw, currentY + ch);
            currentY += ch + lp->bottomMargin;
        }
    }
}

std::shared_ptr<View::LayoutParams> LinearLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
    ViewGroup::parseBaseLayoutParams(lp, parser);
    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        
        std::string rawValue = "";
        size_t valLen;
        const char16_t* val16 = parser->getAttributeStringValue(i, &valLen);
        if (val16) rawValue = android::util::Utf16ToUtf8(android::StringPiece16(val16, valLen));
        
        uint8_t type = parser->getAttributeDataType(i);
        uint32_t data = parser->getAttributeData(i);

        if (attrName == "layout_weight") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->weight = (type == 0x04) ? u.f : (float)data;
        } else if (attrName == "layout_gravity") {
            lp->gravity = data;
        }
    }
    return lp;
}

} // namespace view
} // namespace setu




