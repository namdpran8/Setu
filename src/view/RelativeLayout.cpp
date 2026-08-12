#include "androidfw/Util.h"
#include "RelativeLayout.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "androidfw/ResourceTypes.h"

namespace windroid {
namespace view {

static std::vector<std::shared_ptr<View>> getSortedChildren(const std::vector<std::shared_ptr<View>>& children) {
    std::unordered_map<View*, std::vector<std::shared_ptr<View>>> graph;
    std::unordered_map<View*, int> inDegree;
    std::vector<std::shared_ptr<View>> sorted;
    std::unordered_map<int, std::shared_ptr<View>> idMap;

    for (const auto& child : children) {
        if (child->getId() != 0) {
            idMap[child->getId()] = child;
        }
        inDegree[child.get()] = 0;
    }

    for (const auto& child : children) {
        auto lp = std::dynamic_pointer_cast<RelativeLayout::LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        for (const auto& rule : {RelativeLayout::LayoutParams::ABOVE, RelativeLayout::LayoutParams::BELOW, 
                                 RelativeLayout::LayoutParams::LEFT_OF, RelativeLayout::LayoutParams::RIGHT_OF}) {
            if (lp->rules.count(rule)) {
                int targetId = lp->rules[rule];
                if (idMap.count(targetId)) {
                    auto target = idMap[targetId];
                    graph[target.get()].push_back(child);
                    inDegree[child.get()]++;
                }
            }
        }
    }

    std::vector<std::shared_ptr<View>> queue;
    for (const auto& child : children) {
        if (inDegree[child.get()] == 0) {
            queue.push_back(child);
        }
    }

    while (!queue.empty()) {
        auto curr = queue.front();
        queue.erase(queue.begin());
        sorted.push_back(curr);

        for (const auto& dependent : graph[curr.get()]) {
            inDegree[dependent.get()]--;
            if (inDegree[dependent.get()] == 0) {
                queue.push_back(dependent);
            }
        }
    }

    if (sorted.size() != children.size()) {
        for (const auto& child : children) {
            if (std::find(sorted.begin(), sorted.end(), child) == sorted.end()) {
                sorted.push_back(child);
            }
        }
    }
    return sorted;
}

const std::string RelativeLayout::LayoutParams::ABOVE = "layout_above";
const std::string RelativeLayout::LayoutParams::BELOW = "layout_below";
const std::string RelativeLayout::LayoutParams::LEFT_OF = "layout_toLeftOf";
const std::string RelativeLayout::LayoutParams::RIGHT_OF = "layout_toRightOf";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_LEFT = "layout_alignParentLeft";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_TOP = "layout_alignParentTop";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_RIGHT = "layout_alignParentRight";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_BOTTOM = "layout_alignParentBottom";

std::shared_ptr<View> RelativeLayout::getViewById(int id) {
    for (auto& child : mChildren) {
        if (child->getId() == id) {
            return child;
        }
    }
    return nullptr;
}

void RelativeLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int maxWidth = 0;
    int maxHeight = 0;

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Topological sort pass
    auto sortedChildren = getSortedChildren(mChildren);

    for (auto& child : sortedChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = View::makeMeasureSpec(widthSize, (lp->width == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
        int childHeightSpec = View::makeMeasureSpec(heightSize, (lp->height == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
        if (lp->width > 0) childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
        if (lp->height > 0) childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);

        child->measure(childWidthSpec, childHeightSpec);
    }

    // Measure self
    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : widthSize; // Default to size
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : heightSize;
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void RelativeLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int parentWidth = r - l;
    int parentHeight = b - t;

    auto sortedChildren = getSortedChildren(mChildren);

    // Apply rules to figure out positioning
    for (auto& child : sortedChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();
        
        int childLeft = lp->leftMargin;
        int childTop = lp->topMargin;
        int childRight = childLeft + cw;
        int childBottom = childTop + ch;

        // Apply align parent rules
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_RIGHT)) {
            childRight = parentWidth - lp->rightMargin;
            childLeft = childRight - cw;
        }
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_BOTTOM)) {
            childBottom = parentHeight - lp->bottomMargin;
            childTop = childBottom - ch;
        }

        // Apply relative positioning rules (very naive implementation without topological sort)
        if (lp->rules.count(LayoutParams::BELOW)) {
            auto target = getViewById(lp->rules[LayoutParams::BELOW]);
            if (target) {
                childTop = target->getBottom() + lp->topMargin;
                childBottom = childTop + ch;
            }
        }
        if (lp->rules.count(LayoutParams::ABOVE)) {
            auto target = getViewById(lp->rules[LayoutParams::ABOVE]);
            if (target) {
                childBottom = target->getTop() - lp->bottomMargin;
                childTop = childBottom - ch;
            }
        }
        if (lp->rules.count(LayoutParams::RIGHT_OF)) {
            auto target = getViewById(lp->rules[LayoutParams::RIGHT_OF]);
            if (target) {
                childLeft = target->getRight() + lp->leftMargin;
                childRight = childLeft + cw;
            }
        }
        if (lp->rules.count(LayoutParams::LEFT_OF)) {
            auto target = getViewById(lp->rules[LayoutParams::LEFT_OF]);
            if (target) {
                childRight = target->getLeft() - lp->rightMargin;
                childLeft = childRight - cw;
            }
        }

        child->layout(childLeft, childTop, childRight, childBottom);
    }
}

std::shared_ptr<View::LayoutParams> RelativeLayout::generateLayoutParams(android::ResXMLParser* parser) {
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

        if (attrName == LayoutParams::ABOVE ||
            attrName == LayoutParams::BELOW ||
            attrName == LayoutParams::LEFT_OF ||
            attrName == LayoutParams::RIGHT_OF ||
            attrName == LayoutParams::ALIGN_PARENT_LEFT ||
            attrName == LayoutParams::ALIGN_PARENT_TOP ||
            attrName == LayoutParams::ALIGN_PARENT_RIGHT ||
            attrName == LayoutParams::ALIGN_PARENT_BOTTOM) {
            lp->rules[attrName] = data;
        }
    }
    return lp;
}

} // namespace view
} // namespace windroid




