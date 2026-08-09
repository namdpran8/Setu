#include "LayoutInflater.h"
#include "../utils/Logger.h"
#include <windows.h>
#include <string>
#include <algorithm>

// Helper to convert UTF-8 to UTF-16 for Win32 API
static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

#include "../widget/TextView.h"
#include "../widget/Button.h"
#include "../view/ViewGroup.h"

std::shared_ptr<windroid::view::View> LayoutInflater::inflate(const AxmlNode* node, ResourceManager* resManager, int parentWidth, int parentHeight) {
    ConstraintNode root = inflateRecursive(node, resManager);
    
    // Set root node dimensions
    root.x = 0; root.y = 0; root.w = parentWidth; root.h = parentHeight;
    
    layoutNode(root, parentWidth, parentHeight);
    return root.view;
}

std::string LayoutInflater::resolveString(const AxmlAttribute& attr, ResourceManager* resManager) {
    if (!attr.rawValue.empty()) return attr.rawValue;
    if (attr.typedValueType == 0x01 || attr.typedValueType == 0x03) {
        if (resManager) {
            std::string resolved = resManager->getResourcePath(attr.typedValueData);
            if (!resolved.empty()) return resolved;
        }
    }
    return "";
}

ConstraintNode LayoutInflater::inflateRecursive(const AxmlNode* node, ResourceManager* resManager) {
    ConstraintNode cnode;
    if (!node) return cnode;
    
    std::string tag = node->tag;
    
    cnode.width = 80;
    cnode.height = 70;

    std::string text = "";
    
    if (tag.find("ConstraintLayout") != std::string::npos) cnode.isConstraintLayout = true;
    if (tag.find("LinearLayout") != std::string::npos) {
        cnode.isLinearLayout = true;
        cnode.isHorizontal = true; // Default Android behavior
    }
    if (tag.find("Guideline") != std::string::npos) cnode.isGuideline = true;

    for (const auto& attr : node->attributes) {
        if (attr.name == "text") text = resolveString(attr, resManager);
        else if (attr.name == "id" && attr.typedValueType == 0x01) cnode.id = attr.typedValueData;
        else if (attr.name == "orientation") {
            if (attr.typedValueData == 0) { cnode.isHorizontal = true; cnode.isHorizontalGuide = true; }
            else if (attr.typedValueData == 1) { cnode.isHorizontal = false; cnode.isHorizontalGuide = false; }
        }
        else if (attr.name == "layout_width") {
            int32_t val = (int32_t)attr.typedValueData;
            if (attr.typedValueType == 0x10 || attr.typedValueType == 0x11) {
                if (val == -1) cnode.widthMode = 1; // MATCH_PARENT
                else if (val == -2) cnode.widthMode = 0; // WRAP_CONTENT
            } else if (attr.typedValueType == 0x05) { // DIMENSION
                int unit = val & 0xF;
                int actualVal = val >> 8;
                if (actualVal == 0) cnode.widthMode = 2; // MATCH_CONSTRAINT (0dp)
                else { cnode.widthMode = 0; cnode.width = actualVal; }
            } else {
                cnode.widthMode = 0; cnode.width = val; // Fallback
            }
        }
        else if (attr.name == "layout_height") {
            int32_t val = (int32_t)attr.typedValueData;
            if (attr.typedValueType == 0x10 || attr.typedValueType == 0x11) {
                if (val == -1) cnode.heightMode = 1; // MATCH_PARENT
                else if (val == -2) cnode.heightMode = 0; // WRAP_CONTENT
            } else if (attr.typedValueType == 0x05) { // DIMENSION
                int unit = val & 0xF;
                int actualVal = val >> 8;
                if (actualVal == 0) cnode.heightMode = 2; // MATCH_CONSTRAINT (0dp)
                else { cnode.heightMode = 0; cnode.height = actualVal; }
            } else {
                cnode.heightMode = 0; cnode.height = val; // Fallback
            }
        }
        else if (attr.name == "layout_constraintTop_toTopOf") cnode.topToTop = attr.typedValueData;
        else if (attr.name == "layout_constraintTop_toBottomOf") cnode.topToBottom = attr.typedValueData;
        else if (attr.name == "layout_constraintBottom_toTopOf") cnode.bottomToTop = attr.typedValueData;
        else if (attr.name == "layout_constraintBottom_toBottomOf") cnode.bottomToBottom = attr.typedValueData;
        else if (attr.name == "layout_constraintStart_toStartOf") cnode.startToStart = attr.typedValueData;
        else if (attr.name == "layout_constraintStart_toEndOf") cnode.startToEnd = attr.typedValueData;
        else if (attr.name == "layout_constraintEnd_toStartOf") cnode.endToStart = attr.typedValueData;
        else if (attr.name == "layout_constraintEnd_toEndOf") cnode.endToEnd = attr.typedValueData;
        else if (attr.name == "layout_constraintHorizontal_bias") {
            if (attr.typedValueType == 0x04) cnode.horizontalBias = *(float*)&attr.typedValueData;
        }
        else if (attr.name == "layout_constraintVertical_bias") {
            if (attr.typedValueType == 0x04) cnode.verticalBias = *(float*)&attr.typedValueData;
        }
        else if (attr.name == "layout_constraintGuide_percent") {
            if (attr.typedValueType == 0x04) cnode.guidePercent = *(float*)&attr.typedValueData;
        }
        else if (attr.name == "layout_constraintGuide_begin") cnode.guideBegin = attr.typedValueData;
        else if (attr.name == "layout_constraintGuide_end") cnode.guideEnd = attr.typedValueData;
    }

    if (!cnode.isGuideline) {
        if (tag.find("TextView") != std::string::npos) {
            auto tv = std::make_shared<windroid::widget::TextView>();
            tv->setText(utf8_to_utf16(text));
            cnode.view = tv;
            cnode.width = 300; cnode.height = 60;
        } else if (tag.find("Button") != std::string::npos) {
            auto btn = std::make_shared<windroid::widget::Button>();
            btn->setText(utf8_to_utf16(text));
            cnode.view = btn;
        } else {
            cnode.view = std::make_shared<windroid::view::ViewGroup>();
        }
        
        if (cnode.view) {
            cnode.view->setId(cnode.id);
        }
    }
    
    for (const auto& child : node->children) {
        ConstraintNode childNode = inflateRecursive(child.get(), resManager);
        if (childNode.view || childNode.isGuideline) {
            cnode.children.push_back(childNode);
            if (cnode.view && childNode.view) {
                auto viewGroup = std::dynamic_pointer_cast<windroid::view::ViewGroup>(cnode.view);
                if (viewGroup) {
                    viewGroup->addView(childNode.view);
                }
            }
        }
    }
    
    return cnode;
}

void LayoutInflater::layoutNode(ConstraintNode& node, int parentWidth, int parentHeight) {
    if (node.isConstraintLayout) {
        resolveConstraints(node.children, parentWidth, parentHeight);
        for (auto& child : node.children) {
            layoutNode(child, child.w, child.h);
        }
    } else if (node.isLinearLayout) {
        int currentX = 0, currentY = 0;
        int maxChildHeight = 0, maxChildWidth = 0;
        for (auto& child : node.children) {
            child.x = currentX;
            child.y = currentY;
            child.w = child.width; 
            child.h = child.height;
            if (child.widthMode == 1) child.w = parentWidth;
            if (child.heightMode == 1) child.h = parentHeight;
            
            if (node.isHorizontal) {
                currentX += child.w + 5; 
                maxChildHeight = max(maxChildHeight, child.h);
            } else {
                currentY += child.h + 5;
                maxChildWidth = max(maxChildWidth, child.w);
            }
            if (child.view) {
                child.view->setMeasuredDimension(child.w, child.h);
                child.view->layout(child.x, child.y, child.x + child.w, child.y + child.h);
            }
            layoutNode(child, child.w, child.h);
        }
    } else {
        // Fallback for Generic ViewGroups
        for (auto& child : node.children) {
            child.x = 0; child.y = 0;
            child.w = child.width; child.h = child.height;
            if (child.widthMode == 1) child.w = parentWidth;
            if (child.heightMode == 1) child.h = parentHeight;
            if (child.view) {
                child.view->setMeasuredDimension(child.w, child.h);
                child.view->layout(child.x, child.y, child.x + child.w, child.y + child.h);
            }
            layoutNode(child, child.w, child.h);
        }
    }
    if (node.view) {
        node.view->setMeasuredDimension(node.w, node.h);
        node.view->layout(node.x, node.y, node.x + node.w, node.y + node.h);
    }
}

void LayoutInflater::resolveConstraints(std::vector<ConstraintNode>& nodes, int parentWidth, int parentHeight) {
    size_t maxIterations = nodes.size();
    
    auto getNode = [&](uint32_t id) -> ConstraintNode* {
        if (id == 0 || id == 0xFFFFFFFF) return nullptr;
        for (auto& n : nodes) if (n.id == id) return &n;
        return nullptr;
    };
    
    for (size_t iter = 0; iter < maxIterations; ++iter) {
        bool changed = false;
        
        for (auto& node : nodes) {
            // Guideline Resolution
            if (node.isGuideline) {
                if (!node.resolvedX || !node.resolvedY) {
                    if (node.isHorizontalGuide) {
                        if (node.guidePercent >= 0) node.y = (int)(parentHeight * node.guidePercent);
                        else if (node.guideBegin >= 0) node.y = node.guideBegin;
                        else if (node.guideEnd >= 0) node.y = parentHeight - node.guideEnd;
                        else node.y = 0;
                        node.x = 0; node.w = parentWidth; node.h = 0;
                    } else {
                        if (node.guidePercent >= 0) node.x = (int)(parentWidth * node.guidePercent);
                        else if (node.guideBegin >= 0) node.x = node.guideBegin;
                        else if (node.guideEnd >= 0) node.x = parentWidth - node.guideEnd;
                        else node.x = 0;
                        node.y = 0; node.w = 0; node.h = parentHeight;
                    }
                    node.resolvedX = true; 
                    node.resolvedY = true;
                    changed = true;
                }
                continue;
            }
            
            // X Resolution
            if (!node.resolvedX) {
                bool startReady = (node.startToStart == 0 || node.startToStart == 0xFFFFFFFF || (getNode(node.startToStart) && getNode(node.startToStart)->resolvedX)) &&
                                  (node.startToEnd == 0 || node.startToEnd == 0xFFFFFFFF || (getNode(node.startToEnd) && getNode(node.startToEnd)->resolvedX));
                bool endReady = (node.endToStart == 0 || node.endToStart == 0xFFFFFFFF || (getNode(node.endToStart) && getNode(node.endToStart)->resolvedX)) &&
                                (node.endToEnd == 0 || node.endToEnd == 0xFFFFFFFF || (getNode(node.endToEnd) && getNode(node.endToEnd)->resolvedX));
                                
                if (startReady && endReady) {
                    int startCoord = 0;
                    bool hasStart = false;
                    if (node.startToStart != 0xFFFFFFFF) {
                        if (node.startToStart == 0) startCoord = 0;
                        else if (auto t = getNode(node.startToStart)) startCoord = t->x;
                        hasStart = true;
                    } else if (node.startToEnd != 0xFFFFFFFF) {
                        if (node.startToEnd == 0) startCoord = parentWidth; 
                        else if (auto t = getNode(node.startToEnd)) startCoord = t->x + t->w;
                        hasStart = true;
                    }

                    int endCoord = parentWidth;
                    bool hasEnd = false;
                    if (node.endToStart != 0xFFFFFFFF) {
                        if (node.endToStart == 0) endCoord = 0;
                        else if (auto t = getNode(node.endToStart)) endCoord = t->x;
                        hasEnd = true;
                    } else if (node.endToEnd != 0xFFFFFFFF) {
                        if (node.endToEnd == 0) endCoord = parentWidth;
                        else if (auto t = getNode(node.endToEnd)) endCoord = t->x + t->w;
                        hasEnd = true;
                    }
                    
                    if (node.widthMode == 2) { // MATCH_CONSTRAINT
                        if (hasStart && hasEnd) node.w = max(0, endCoord - startCoord);
                        else node.w = node.width;
                    } else if (node.widthMode == 1) node.w = parentWidth; // MATCH_PARENT
                    else node.w = node.width; // WRAP_CONTENT

                    if (hasStart && hasEnd) {
                        int available = (endCoord - startCoord) - node.w;
                        node.x = startCoord + (int)(available * node.horizontalBias);
                    } else if (hasStart) node.x = startCoord;
                    else if (hasEnd) node.x = endCoord - node.w;
                    else node.x = 0;
                    
                    node.resolvedX = true;
                    changed = true;
                }
            }
            
            // Y Resolution
            if (!node.resolvedY) {
                bool topReady = (node.topToTop == 0 || node.topToTop == 0xFFFFFFFF || (getNode(node.topToTop) && getNode(node.topToTop)->resolvedY)) &&
                                (node.topToBottom == 0 || node.topToBottom == 0xFFFFFFFF || (getNode(node.topToBottom) && getNode(node.topToBottom)->resolvedY));
                bool bottomReady = (node.bottomToTop == 0 || node.bottomToTop == 0xFFFFFFFF || (getNode(node.bottomToTop) && getNode(node.bottomToTop)->resolvedY)) &&
                                   (node.bottomToBottom == 0 || node.bottomToBottom == 0xFFFFFFFF || (getNode(node.bottomToBottom) && getNode(node.bottomToBottom)->resolvedY));
                                
                if (topReady && bottomReady) {
                    int topCoord = 0;
                    bool hasTop = false;
                    if (node.topToTop != 0xFFFFFFFF) {
                        if (node.topToTop == 0) topCoord = 0;
                        else if (auto t = getNode(node.topToTop)) topCoord = t->y;
                        hasTop = true;
                    } else if (node.topToBottom != 0xFFFFFFFF) {
                        if (node.topToBottom == 0) topCoord = parentHeight; 
                        else if (auto t = getNode(node.topToBottom)) topCoord = t->y + t->h;
                        hasTop = true;
                    }

                    int bottomCoord = parentHeight;
                    bool hasBottom = false;
                    if (node.bottomToTop != 0xFFFFFFFF) {
                        if (node.bottomToTop == 0) bottomCoord = 0;
                        else if (auto t = getNode(node.bottomToTop)) bottomCoord = t->y;
                        hasBottom = true;
                    } else if (node.bottomToBottom != 0xFFFFFFFF) {
                        if (node.bottomToBottom == 0) bottomCoord = parentHeight;
                        else if (auto t = getNode(node.bottomToBottom)) bottomCoord = t->y + t->h;
                        hasBottom = true;
                    }
                    
                    if (node.heightMode == 2) { // MATCH_CONSTRAINT
                        if (hasTop && hasBottom) node.h = max(0, bottomCoord - topCoord);
                        else node.h = node.height;
                    } else if (node.heightMode == 1) node.h = parentHeight; // MATCH_PARENT
                    else node.h = node.height; // WRAP_CONTENT

                    if (hasTop && hasBottom) {
                        int available = (bottomCoord - topCoord) - node.h;
                        node.y = topCoord + (int)(available * node.verticalBias);
                    } else if (hasTop) node.y = topCoord;
                    else if (hasBottom) node.y = bottomCoord - node.h;
                    else node.y = 0;
                    
                    node.resolvedY = true;
                    changed = true;
                }
            }
        }
        
        if (!changed) break; 
    }
    
    for (auto& node : nodes) {
        if (!node.resolvedX || !node.resolvedY) {
            Logger::w("LayoutInflater", "Cycle or unresolved constraint for ID: " + std::to_string(node.id));
            if (!node.resolvedX) { node.x = 0; node.w = node.width; node.resolvedX = true; }
            if (!node.resolvedY) { node.y = 0; node.h = node.height; node.resolvedY = true; }
        }
        
        if (node.view) {
            Logger::d("LayoutInflater", "Layout ID " + std::to_string(node.id) + " -> x: " + std::to_string(node.x) + ", y: " + std::to_string(node.y) + ", w: " + std::to_string(node.w) + ", h: " + std::to_string(node.h));
            node.view->setMeasuredDimension(node.w, node.h);
            node.view->layout(node.x, node.y, node.x + node.w, node.y + node.h);
        }
    }
}
