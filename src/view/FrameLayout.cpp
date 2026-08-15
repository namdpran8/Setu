#include "androidfw/Util.h"
#include "FrameLayout.h"
#include <algorithm>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

void FrameLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int maxWidth = 0;
    int maxHeight = 0;

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        measureChildWithMargins(child, widthMeasureSpec, 0, heightMeasureSpec, 0);

        auto lp = child->getLayoutParams();

        maxWidth = std::max(maxWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        maxHeight = std::max(maxHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
    }

    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : maxWidth;
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : maxHeight;

    if (widthMode == MEASURE_SPEC_AT_MOST) measuredWidth = std::min(measuredWidth, widthSize);
    if (heightMode == MEASURE_SPEC_AT_MOST) measuredHeight = std::min(measuredHeight, heightSize);

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void FrameLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int parentWidth = r - l;
    int parentHeight = b - t;

    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        // Fix gravity
        int gravity = lp->gravity;
        if (gravity == -1 || gravity == 0) gravity = 0x33; // TOP | LEFT

        int horizontalGravity = gravity & 0x07;
        int verticalGravity = gravity & 0x70;

        int childLeft = lp->leftMargin;
        int childTop = lp->topMargin;

        switch (horizontalGravity) {
            case 0x01: // CENTER_HORIZONTAL
                childLeft = (parentWidth - cw - lp->leftMargin - lp->rightMargin) / 2 + lp->leftMargin;
                break;
            case 0x05: // RIGHT
                childLeft = parentWidth - cw - lp->rightMargin;
                break;
            case 0x03: // LEFT
            default:
                childLeft = lp->leftMargin;
                break;
        }

        switch (verticalGravity) {
            case 0x10: // CENTER_VERTICAL
                childTop = (parentHeight - ch - lp->topMargin - lp->bottomMargin) / 2 + lp->topMargin;
                break;
            case 0x50: // BOTTOM
                childTop = parentHeight - ch - lp->bottomMargin;
                break;
            case 0x30: // TOP
            default:
                childTop = lp->topMargin;
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




