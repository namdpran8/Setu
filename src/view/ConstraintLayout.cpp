#include "androidfw/Util.h"
#include "ConstraintLayout.h"
#include <algorithm>
#include "../utils/Logger.h"
#include "androidfw/ResourceTypes.h"

namespace windroid {
namespace view {

void ConstraintLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Initial pass: populate ResolvedNodes
    mResolvedNodes.clear();
    mIdToIndex.clear();
    for (auto& child : mChildren) {
        ResolvedNode node;
        node.view = child;
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
        
        // Measure wrap_content/exactly up front to get intrinsic sizes
        if (lp->width != 0) {
            int cWidthSpec = View::makeMeasureSpec(widthSize, lp->width == View::MATCH_PARENT ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
            if (lp->width > 0) cWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
            
            int cHeightSpec = View::makeMeasureSpec(heightSize, lp->height == View::MATCH_PARENT ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
            if (lp->height > 0) cHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
            
            child->measure(cWidthSpec, cHeightSpec);
            node.w = child->getMeasuredWidth();
            node.h = child->getMeasuredHeight();
        } else {
            node.w = 0;
            node.h = 0;
        }

        mResolvedNodes.push_back(node);
        int id = child->getId();
        if (id != -1 && id != 0) {
            mIdToIndex[id] = mResolvedNodes.size() - 1;
        }
    }

    // Resolve positions and MATCH_CONSTRAINT sizes
    resolveConstraints(widthSize, heightSize);

    // After resolving, apply measurement
    int maxWidth = 0;
    int maxHeight = 0;
    for (auto& node : mResolvedNodes) {
        auto child = node.view;
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        
        // Remeasure if it was MATCH_CONSTRAINT
        if (lp && (lp->width == 0 || lp->height == 0)) {
            child->measure(View::makeMeasureSpec(node.w, View::MEASURE_SPEC_EXACTLY), View::makeMeasureSpec(node.h, View::MEASURE_SPEC_EXACTLY));
        }

        maxWidth = std::max(maxWidth, node.x + node.w);
        maxHeight = std::max(maxHeight, node.y + node.h);
    }

    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : maxWidth;
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : maxHeight;

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void ConstraintLayout::resolveConstraints(int parentWidth, int parentHeight) {
    size_t maxIterations = mResolvedNodes.size();
    
    auto getNode = [&](int id) -> ResolvedNode* {
        if (id == 0 || id == -1) return nullptr;
        auto it = mIdToIndex.find(id);
        if (it != mIdToIndex.end()) return &mResolvedNodes[it->second];
        return nullptr;
    };
    
    for (size_t iter = 0; iter < maxIterations; ++iter) {
        bool changed = false;
        
        for (auto& node : mResolvedNodes) {
            auto lp = std::dynamic_pointer_cast<LayoutParams>(node.view->getLayoutParams());
            if (!lp) continue;

            // Guideline Resolution
            if (lp->isGuideline) {
                if (!node.xResolved || !node.yResolved) {
                    if (lp->orientation == 0) { // Horizontal Guide
                        if (lp->guidePercent >= 0) node.y = (int)(parentHeight * lp->guidePercent);
                        else if (lp->guideBegin >= 0) node.y = lp->guideBegin;
                        else if (lp->guideEnd >= 0) node.y = parentHeight - lp->guideEnd;
                        else node.y = 0;
                        node.x = 0; node.w = parentWidth; node.h = 0;
                    } else { // Vertical Guide
                        if (lp->guidePercent >= 0) node.x = (int)(parentWidth * lp->guidePercent);
                        else if (lp->guideBegin >= 0) node.x = lp->guideBegin;
                        else if (lp->guideEnd >= 0) node.x = parentWidth - lp->guideEnd;
                        else node.x = 0;
                        node.y = 0; node.w = 0; node.h = parentHeight;
                    }
                    node.xResolved = true; 
                    node.yResolved = true;
                    changed = true;
                }
                continue;
            }
            
            // X Resolution
            if (!node.xResolved) {
                bool startReady = (lp->startToStart == 0 || lp->startToStart == -1 || (getNode(lp->startToStart) && getNode(lp->startToStart)->xResolved)) &&
                                  (lp->startToEnd == 0 || lp->startToEnd == -1 || (getNode(lp->startToEnd) && getNode(lp->startToEnd)->xResolved));
                bool endReady = (lp->endToStart == 0 || lp->endToStart == -1 || (getNode(lp->endToStart) && getNode(lp->endToStart)->xResolved)) &&
                                (lp->endToEnd == 0 || lp->endToEnd == -1 || (getNode(lp->endToEnd) && getNode(lp->endToEnd)->xResolved));
                                
                if (startReady && endReady) {
                    int startCoord = 0;
                    bool hasStart = false;
                    if (lp->startToStart != -1) {
                        if (lp->startToStart == 0) startCoord = 0;
                        else if (auto t = getNode(lp->startToStart)) startCoord = t->x;
                        hasStart = true;
                    } else if (lp->startToEnd != -1) {
                        if (lp->startToEnd == 0) startCoord = parentWidth; 
                        else if (auto t = getNode(lp->startToEnd)) startCoord = t->x + t->w;
                        hasStart = true;
                    }
                    startCoord += lp->leftMargin;

                    int endCoord = parentWidth;
                    bool hasEnd = false;
                    if (lp->endToStart != -1) {
                        if (lp->endToStart == 0) endCoord = 0;
                        else if (auto t = getNode(lp->endToStart)) endCoord = t->x;
                        hasEnd = true;
                    } else if (lp->endToEnd != -1) {
                        if (lp->endToEnd == 0) endCoord = parentWidth;
                        else if (auto t = getNode(lp->endToEnd)) endCoord = t->x + t->w;
                        hasEnd = true;
                    }
                    endCoord -= lp->rightMargin;
                    
                    if (lp->width == 0) { // MATCH_CONSTRAINT
                        if (hasStart && hasEnd) node.w = std::max(0, endCoord - startCoord);
                    } else if (lp->width == View::MATCH_PARENT) {
                        node.w = parentWidth - lp->leftMargin - lp->rightMargin;
                    }

                    if (hasStart && hasEnd) {
                        int available = (endCoord - startCoord) - node.w;
                        node.x = startCoord + (int)(available * lp->horizontalBias);
                    } else if (hasStart) node.x = startCoord;
                    else if (hasEnd) node.x = endCoord - node.w;
                    else node.x = lp->leftMargin;
                    
                    node.xResolved = true;
                    changed = true;
                }
            }
            
            // Y Resolution
            if (!node.yResolved) {
                bool topReady = (lp->topToTop == 0 || lp->topToTop == -1 || (getNode(lp->topToTop) && getNode(lp->topToTop)->yResolved)) &&
                                (lp->topToBottom == 0 || lp->topToBottom == -1 || (getNode(lp->topToBottom) && getNode(lp->topToBottom)->yResolved));
                bool bottomReady = (lp->bottomToTop == 0 || lp->bottomToTop == -1 || (getNode(lp->bottomToTop) && getNode(lp->bottomToTop)->yResolved)) &&
                                   (lp->bottomToBottom == 0 || lp->bottomToBottom == -1 || (getNode(lp->bottomToBottom) && getNode(lp->bottomToBottom)->yResolved));
                                
                if (topReady && bottomReady) {
                    int topCoord = 0;
                    bool hasTop = false;
                    if (lp->topToTop != -1) {
                        if (lp->topToTop == 0) topCoord = 0;
                        else if (auto t = getNode(lp->topToTop)) topCoord = t->y;
                        hasTop = true;
                    } else if (lp->topToBottom != -1) {
                        if (lp->topToBottom == 0) topCoord = parentHeight; 
                        else if (auto t = getNode(lp->topToBottom)) topCoord = t->y + t->h;
                        hasTop = true;
                    }
                    topCoord += lp->topMargin;

                    int bottomCoord = parentHeight;
                    bool hasBottom = false;
                    if (lp->bottomToTop != -1) {
                        if (lp->bottomToTop == 0) bottomCoord = 0;
                        else if (auto t = getNode(lp->bottomToTop)) bottomCoord = t->y;
                        hasBottom = true;
                    } else if (lp->bottomToBottom != -1) {
                        if (lp->bottomToBottom == 0) bottomCoord = parentHeight;
                        else if (auto t = getNode(lp->bottomToBottom)) bottomCoord = t->y + t->h;
                        hasBottom = true;
                    }
                    bottomCoord -= lp->bottomMargin;
                    
                    if (lp->height == 0) { // MATCH_CONSTRAINT
                        if (hasTop && hasBottom) node.h = std::max(0, bottomCoord - topCoord);
                    } else if (lp->height == View::MATCH_PARENT) {
                        node.h = parentHeight - lp->topMargin - lp->bottomMargin;
                    }

                    if (hasTop && hasBottom) {
                        int available = (bottomCoord - topCoord) - node.h;
                        node.y = topCoord + (int)(available * lp->verticalBias);
                    } else if (hasTop) node.y = topCoord;
                    else if (hasBottom) node.y = bottomCoord - node.h;
                    else node.y = lp->topMargin;
                    
                    node.yResolved = true;
                    changed = true;
                }
            }
        }
        
        if (!changed) break; 
    }
    
    // Fallback for unresolved cycles
    for (auto& node : mResolvedNodes) {
        if (!node.xResolved || !node.yResolved) {
            Logger::w("ConstraintLayout", "Cycle or unresolved constraint for ID: " + std::to_string(node.view->getId()));
            if (!node.xResolved) { node.x = 0; node.xResolved = true; }
            if (!node.yResolved) { node.y = 0; node.yResolved = true; }
        }
    }
}

void ConstraintLayout::onLayout(bool changed, int l, int t, int r, int b) {
    for (auto& node : mResolvedNodes) {
        node.view->layout(node.x, node.y, node.x + node.w, node.y + node.h);
    }
}

std::shared_ptr<View::LayoutParams> ConstraintLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
    ViewGroup::parseBaseLayoutParams(lp, parser);

    auto parseConstraintTarget = [&](uint8_t type, uint32_t data, const std::string& rawValue) {
        if (type == android::Res_value::TYPE_STRING) {
            if (rawValue == "parent") return 0;
        }
        if (data == 0) return 0;
        return (int)data;
    };

    size_t tagLen;
    const char16_t* tag16 = parser->getElementName(&tagLen);
    if (tag16) {
        std::string tagStr = android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen));
        if (tagStr.find("Guideline") != std::string::npos) lp->isGuideline = true;
    }

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

        if (attrName == "layout_constraintTop_toTopOf") lp->topToTop = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintTop_toBottomOf") lp->topToBottom = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintBottom_toTopOf") lp->bottomToTop = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintBottom_toBottomOf") lp->bottomToBottom = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintStart_toStartOf" || attrName == "layout_constraintLeft_toLeftOf") lp->startToStart = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintStart_toEndOf" || attrName == "layout_constraintLeft_toRightOf") lp->startToEnd = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintEnd_toStartOf" || attrName == "layout_constraintRight_toLeftOf") lp->endToStart = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintEnd_toEndOf" || attrName == "layout_constraintRight_toRightOf") lp->endToEnd = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintHorizontal_bias") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->horizontalBias = u.f;
        }
        else if (attrName == "layout_constraintVertical_bias") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->verticalBias = u.f;
        }
        else if (attrName == "layout_constraintGuide_percent") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->guidePercent = u.f;
        }
        else if (attrName == "layout_constraintGuide_begin") lp->guideBegin = (data >> 8) * 2;
        else if (attrName == "layout_constraintGuide_end") lp->guideEnd = (data >> 8) * 2;
        else if (attrName == "orientation") lp->orientation = data;
    }
    return lp;
}

} // namespace view
} // namespace windroid




