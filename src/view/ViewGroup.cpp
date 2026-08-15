#include "androidfw/Util.h"
#include "ViewGroup.h"
#include <algorithm>
#include "MotionEvent.h"
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

std::shared_ptr<View> ViewGroup::findViewById(int targetId) {
    if (mId == targetId) return shared_from_this();
    for (auto& child : mChildren) {
        if (auto found = child->findViewById(targetId)) return found;
    }
    return nullptr;
}

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
        if (child->getVisibility() == View::VISIBLE) {
            child->updateRenderNode();
            canvas.drawRenderNode(child->getRenderNode());
        }
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

bool ViewGroup::dispatchKeyEvent(const KeyEvent& event) {
    // Traverse children to find the focused one that handles the event.
    // We rely on the child's own dispatchKeyEvent/onKeyEvent to check focus.
    for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it) {
        if ((*it)->dispatchKeyEvent(event)) return true;
    }
    // If no child handled it (or none focused), try handling it ourselves
    return View::dispatchKeyEvent(event);
}

int ViewGroup::getChildMeasureSpec(int spec, int padding, int childDimension) {
    int specMode = View::getMode(spec);
    int specSize = View::getSize(spec);
    int size = std::max(0, specSize - padding);

    int resultSize = 0;
    int resultMode = 0;

    if (specMode == View::MEASURE_SPEC_EXACTLY) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        }
    } else if (specMode == View::MEASURE_SPEC_AT_MOST) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        }
    } else if (specMode == View::MEASURE_SPEC_UNSPECIFIED) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_UNSPECIFIED;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_UNSPECIFIED;
        }
    }
    return View::makeMeasureSpec(resultSize, resultMode);
}

void ViewGroup::measureChild(std::shared_ptr<View> child, int parentWidthMeasureSpec, int parentHeightMeasureSpec) {
    auto lp = child->getLayoutParams();
    if (!lp) return;

    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec, 0, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec, 0, lp->height);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::measureChildWithMargins(std::shared_ptr<View> child, 
        int parentWidthMeasureSpec, int widthUsed,
        int parentHeightMeasureSpec, int heightUsed) {
    auto lp = child->getLayoutParams();
    if (!lp) return;

    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec,
            widthUsed + lp->leftMargin + lp->rightMargin, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec,
            heightUsed + lp->topMargin + lp->bottomMargin, lp->height);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::parseBaseLayoutParams(std::shared_ptr<View::LayoutParams> lp, android::ResXMLParser* parser) {
    if (!parser || !lp) return;
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

        if (attrName == "layout_width") {
            if (type == 0x03) { // STRING
                if (rawValue == "match_parent" || rawValue == "fill_parent") lp->width = View::MATCH_PARENT;
                else if (rawValue == "wrap_content") lp->width = View::WRAP_CONTENT;
            } else if (type == 0x10 || type == 0x11) { // INT
                if ((int)data == -1) lp->width = View::MATCH_PARENT;
                else if ((int)data == -2) lp->width = View::WRAP_CONTENT;
                else lp->width = data;
            } else if (type == 0x05) { // DIMENSION
                lp->width = data; // Raw for now, should map px
            }
        } else if (attrName == "layout_height") {
            if (type == 0x03) { 
                if (rawValue == "match_parent" || rawValue == "fill_parent") lp->height = View::MATCH_PARENT;
                else if (rawValue == "wrap_content") lp->height = View::WRAP_CONTENT;
            } else if (type == 0x10 || type == 0x11) { // INT
                if ((int)data == -1) lp->height = View::MATCH_PARENT;
                else if ((int)data == -2) lp->height = View::WRAP_CONTENT;
                else lp->height = data;
            } else if (type == 0x05) { // DIMENSION
                lp->height = data;
            }
        } else if (attrName == "layout_marginLeft" || attrName == "layout_marginStart") {
            lp->leftMargin = data;
        } else if (attrName == "layout_marginRight" || attrName == "layout_marginEnd") {
            lp->rightMargin = data;
        } else if (attrName == "layout_marginTop") {
            lp->topMargin = data;
        } else if (attrName == "layout_marginBottom") {
            lp->bottomMargin = data;
        }
    }
}

std::shared_ptr<View::LayoutParams> ViewGroup::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<View::LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    parseBaseLayoutParams(lp, parser);
    return lp;
}

void ViewGroup::dump(int depth) {
    View::dump(depth);
    for (auto& child : mChildren) {
        child->dump(depth + 1);
    }
}

} // namespace view
} // namespace setu





