#include "androidfw/Util.h"
#include "LinearLayout.h"
#include <algorithm>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

void LinearLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);
    
    bool isVertical = (mOrientation == Orientation::VERTICAL);
    
    int totalWeight = 0;
    int usedSize = 0;
    int maxOrthogonal = 0;
    
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;
        
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
        
        int childWidthSpec, childHeightSpec;
        
        if (isVertical) {
            childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
            if (lp->weight > 0) {
                totalWeight += lp->weight;
                childHeightSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
            } else {
                childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
            }
        } else {
            childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
            if (lp->weight > 0) {
                totalWeight += lp->weight;
                childWidthSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
            } else {
                childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
            }
        }
        
        child->measure(childWidthSpec, childHeightSpec);
        
        if (isVertical) {
            usedSize += child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
            maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        } else {
            usedSize += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
            maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
        }
    }
    
    if (totalWeight > 0) {
        if (isVertical) {
            int remaining = heightSize - mPaddingTop - mPaddingBottom - usedSize;
            if (remaining < 0) remaining = 0;
            
            for (auto& child : mChildren) {
                if (child->getVisibility() == View::GONE) continue;
                auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
                if (lp && lp->weight > 0) {
                    int childHeight = (remaining * lp->weight) / totalWeight;
                    int childHeightSpec = View::makeMeasureSpec(childHeight, View::MEASURE_SPEC_EXACTLY);
                    int childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
                    child->measure(childWidthSpec, childHeightSpec);
                    usedSize += childHeight + lp->topMargin + lp->bottomMargin;
                    maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
                }
            }
        } else {
            int remaining = widthSize - mPaddingLeft - mPaddingRight - usedSize;
            if (remaining < 0) remaining = 0;
            
            for (auto& child : mChildren) {
                if (child->getVisibility() == View::GONE) continue;
                auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
                if (lp && lp->weight > 0) {
                    int childWidth = (remaining * lp->weight) / totalWeight;
                    int childWidthSpec = View::makeMeasureSpec(childWidth, View::MEASURE_SPEC_EXACTLY);
                    int childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
                    child->measure(childWidthSpec, childHeightSpec);
                    usedSize += childWidth + lp->leftMargin + lp->rightMargin;
                    maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
                }
            }
        }
    }
    
    int totalWidth = isVertical ? (mPaddingLeft + mPaddingRight + maxOrthogonal) : (mPaddingLeft + mPaddingRight + usedSize);
    int totalHeight = isVertical ? (mPaddingTop + mPaddingBottom + usedSize) : (mPaddingTop + mPaddingBottom + maxOrthogonal);
    
    int measuredWidth = resolveSize(totalWidth, widthMeasureSpec);
    int measuredHeight = resolveSize(totalHeight, heightMeasureSpec);
    
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void LinearLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int childTop = mPaddingTop;
    int childLeft = mPaddingLeft;
    int width = r - l;
    int height = b - t;
    
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;
        
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
        
        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();
        
        int gravity = lp->gravity >= 0 ? lp->gravity : mGravity;
        
        int childLeftFinal = childLeft + lp->leftMargin;
        int childTopFinal = childTop + lp->topMargin;
        
        if (mOrientation == Orientation::VERTICAL) {
            int availWidth = width - mPaddingLeft - mPaddingRight - lp->leftMargin - lp->rightMargin;
            int horizontalGravity = gravity & 0x07;
            if (horizontalGravity == 0x01) { // CENTER_HORIZONTAL
                childLeftFinal += (availWidth - cw) / 2;
            } else if (horizontalGravity == 0x05) { // RIGHT
                childLeftFinal += availWidth - cw;
            }
            
            child->layout(childLeftFinal, childTopFinal, childLeftFinal + cw, childTopFinal + ch);
            childTop = childTopFinal + ch + lp->bottomMargin;
        } else {
            int availHeight = height - mPaddingTop - mPaddingBottom - lp->topMargin - lp->bottomMargin;
            int verticalGravity = gravity & 0x70;
            if (verticalGravity == 0x10) { // CENTER_VERTICAL
                childTopFinal += (availHeight - ch) / 2;
            } else if (verticalGravity == 0x50) { // BOTTOM
                childTopFinal += availHeight - ch;
            }
            
            child->layout(childLeftFinal, childTopFinal, childLeftFinal + cw, childTopFinal + ch);
            childLeft = childLeftFinal + cw + lp->rightMargin;
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
            if (type == 0x04) { // FLOAT
                union { uint32_t i; float f; } u;
                u.i = data;
                lp->weight = u.f;
            } else if (type == 0x03 && val16) { // STRING
                try { lp->weight = std::stof(rawValue); } catch(...) {}
            } else {
                lp->weight = (float)data;
            }
        } else if (attrName == "layout_gravity") {
            if (type == 0x03 && val16) {
                int g = 0;
                if (rawValue.find("center") != std::string::npos) g |= 0x11;
                if (rawValue.find("bottom") != std::string::npos) g |= 0x50;
                if (rawValue.find("right") != std::string::npos || rawValue.find("end") != std::string::npos) g |= 0x05;
                lp->gravity = g;
            } else {
                lp->gravity = data;
            }
        }
    }
    return lp;
}

} // namespace view
} // namespace setu




