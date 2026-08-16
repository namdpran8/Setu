#include "androidfw/Util.h"
#include "FrameLayout.h"
#include <algorithm>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

void FrameLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);
    
    int maxWidth = 0, maxHeight = 0;
    
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;
        
        auto lp = child->getLayoutParams();
        if (!lp) continue;
        
        int childWidthSpec = 0;
        int childHeightSpec = 0;
        
        // Width
        if (lp->width == View::MATCH_PARENT) {
            childWidthSpec = View::makeMeasureSpec(widthSize - mPaddingLeft - mPaddingRight, View::MEASURE_SPEC_EXACTLY);
        } else if (lp->width == View::WRAP_CONTENT) {
            childWidthSpec = View::makeMeasureSpec(widthSize - mPaddingLeft - mPaddingRight, View::MEASURE_SPEC_AT_MOST);
        } else {
            childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
        }
        
        // Height
        if (lp->height == View::MATCH_PARENT) {
            childHeightSpec = View::makeMeasureSpec(heightSize - mPaddingTop - mPaddingBottom, View::MEASURE_SPEC_EXACTLY);
        } else if (lp->height == View::WRAP_CONTENT) {
            childHeightSpec = View::makeMeasureSpec(heightSize - mPaddingTop - mPaddingBottom, View::MEASURE_SPEC_AT_MOST);
        } else {
            childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
        }
        
        child->measure(childWidthSpec, childHeightSpec);
        
        maxWidth = std::max(maxWidth, child->getMeasuredWidth());
        maxHeight = std::max(maxHeight, child->getMeasuredHeight());
    }
    
    maxWidth += mPaddingLeft + mPaddingRight;
    maxHeight += mPaddingTop + mPaddingBottom;
    
    int measuredWidth = resolveSize(maxWidth, widthMeasureSpec);
    int measuredHeight = resolveSize(maxHeight, heightMeasureSpec);
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void FrameLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int parentWidth = r - l;
    int parentHeight = b - t;

    int availWidth = parentWidth - mPaddingLeft - mPaddingRight;
    int availHeight = parentHeight - mPaddingTop - mPaddingBottom;

    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        int gravity = lp->gravity;
        if (gravity == -1 || gravity == 0) gravity = 0x33; // TOP | LEFT

        int horizontalGravity = gravity & 0x07;
        int verticalGravity = gravity & 0x70;

        int childLeft = mPaddingLeft + lp->leftMargin;
        int childTop = mPaddingTop + lp->topMargin;

        switch (horizontalGravity) {
            case 0x01: // CENTER_HORIZONTAL
                childLeft = mPaddingLeft + (availWidth - cw - lp->leftMargin - lp->rightMargin) / 2 + lp->leftMargin;
                break;
            case 0x05: // RIGHT
                childLeft = parentWidth - mPaddingRight - cw - lp->rightMargin;
                break;
            case 0x03: // LEFT
            default:
                childLeft = mPaddingLeft + lp->leftMargin;
                break;
        }

        switch (verticalGravity) {
            case 0x10: // CENTER_VERTICAL
                childTop = mPaddingTop + (availHeight - ch - lp->topMargin - lp->bottomMargin) / 2 + lp->topMargin;
                break;
            case 0x50: // BOTTOM
                childTop = parentHeight - mPaddingBottom - ch - lp->bottomMargin;
                break;
            case 0x30: // TOP
            default:
                childTop = mPaddingTop + lp->topMargin;
                break;
        }

        child->layout(childLeft, childTop, childLeft + cw, childTop + ch);
    }
}

std::shared_ptr<View::LayoutParams> FrameLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
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

        if (attrName == "layout_gravity") {
            lp->gravity = data;
        }
    }
    return lp;
}

} // namespace view
} // namespace setu




