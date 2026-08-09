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

void LayoutInflater::inflate(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager) {
    static HFONT hFont = nullptr;
    if (!hFont) {
        hFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    }

    int currentX = 20; // Default margin
    int currentY = 20; 
    int outW = 0;
    int outH = 0;
    
    std::vector<ConstraintNode> constraintNodes;
    
    inflateRecursive(node, parentHwnd, resManager, currentX, currentY, false, hFont, outW, outH, &constraintNodes);
    
    if (!constraintNodes.empty()) {
        RECT rect;
        GetClientRect(parentHwnd, &rect);
        int pW = rect.right > 0 ? rect.right : 400;
        int pH = rect.bottom > 0 ? rect.bottom : 800;
        resolveConstraints(constraintNodes, pW, pH);
    }
}

HWND LayoutInflater::createDynamicView(const std::string& className, HWND parentHwnd) {
    std::string win32Class = "";
    DWORD style = WS_CHILD | WS_VISIBLE; 
    DWORD exStyle = 0;
    
    if (className.find("TextView") != std::string::npos) {
        win32Class = "STATIC";
    } else if (className.find("Button") != std::string::npos) {
        win32Class = "BUTTON";
    } else if (className.find("EditText") != std::string::npos) {
        win32Class = "EDIT";
        style |= WS_BORDER;
        exStyle |= WS_EX_CLIENTEDGE;
    } else {
        win32Class = "STATIC"; 
    }

    std::wstring wWin32Class = utf8_to_utf16(win32Class);
    HWND hwnd = CreateWindowExW(exStyle, wWin32Class.c_str(), L"", style, 0, 0, 100, 100, parentHwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    
    if (hwnd) {
        static HFONT hFont = nullptr;
        if (!hFont) {
            hFont = CreateFontW(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        }
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
    return hwnd;
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

void LayoutInflater::inflateRecursive(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager, int& currentX, int& currentY, bool isParentHorizontal, HFONT hFont, int& outWidth, int& outHeight, std::vector<ConstraintNode>* constraintNodes) {
    if (!node) {
        outWidth = 0;
        outHeight = 0;
        return;
    }
    
    std::string tag = node->tag;
    std::string win32Class = "";
    DWORD style = WS_CHILD | WS_VISIBLE;
    DWORD exStyle = 0;
    
    if (tag.find("TextView") != std::string::npos) win32Class = "STATIC";
    else if (tag.find("Button") != std::string::npos) win32Class = "BUTTON";
    else if (tag.find("EditText") != std::string::npos) { win32Class = "EDIT"; style |= WS_BORDER; exStyle |= WS_EX_CLIENTEDGE; }
    else if (tag.find("ImageView") != std::string::npos) win32Class = "STATIC";

    ConstraintNode cnode;
    cnode.width = 80;
    cnode.height = 70;
    
    if (win32Class == "STATIC" || win32Class == "EDIT") {
        cnode.width = 300;
        cnode.height = 60;
    }

    std::string text = "";
    bool isHorizontal = false; 
    
    if (tag.find("LinearLayout") != std::string::npos) isHorizontal = true; 

    for (const auto& attr : node->attributes) {
        if (attr.name == "text") text = resolveString(attr, resManager);
        else if (attr.name == "id" && attr.typedValueType == 0x01) cnode.id = attr.typedValueData;
        else if (attr.name == "orientation") {
            if (attr.typedValueData == 0) isHorizontal = true;
            else if (attr.typedValueData == 1) isHorizontal = false;
        }
        else if (attr.name == "layout_width") {
            int32_t val = (int32_t)attr.typedValueData;
            if (val == -1) cnode.widthMode = 1;
            else if (val == -2) cnode.widthMode = 0;
            else if (val == 0) cnode.widthMode = 2;
            else { cnode.widthMode = 0; cnode.width = val; }
        }
        else if (attr.name == "layout_height") {
            int32_t val = (int32_t)attr.typedValueData;
            if (val == -1) cnode.heightMode = 1;
            else if (val == -2) cnode.heightMode = 0;
            else if (val == 0) cnode.heightMode = 2;
            else { cnode.heightMode = 0; cnode.height = val; }
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
    }

    bool isDummy = win32Class.empty();
    if (isDummy) win32Class = "STATIC";

    if (!win32Class.empty() && !isDummy) {
        std::wstring wWin32Class = utf8_to_utf16(win32Class);
        std::wstring wText = utf8_to_utf16(text);
        
        cnode.hwnd = CreateWindowExW(exStyle, wWin32Class.c_str(), wText.c_str(), style, 
            0, 0, cnode.width, cnode.height, parentHwnd, (HMENU)(INT_PTR)cnode.id, GetModuleHandle(nullptr), nullptr);
        
        if (cnode.hwnd) SendMessage(cnode.hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        
        if (constraintNodes) constraintNodes->push_back(cnode);
    }

    int childStartX = currentX;
    int childStartY = currentY;
    int childrenMaxWidth = 0, childrenMaxHeight = 0;
    
    for (const auto& child : node->children) {
        int childW = 0, childH = 0;
        inflateRecursive(child.get(), parentHwnd, resManager, childStartX, childStartY, isHorizontal, hFont, childW, childH, constraintNodes);
        
        if (!constraintNodes) { // fallback flow if no constraint solver requested (though we always pass it now)
            if (isHorizontal) {
                childStartX += childW + 5;
                childrenMaxHeight = std::max(childrenMaxHeight, childH);
                childrenMaxWidth += childW + 5;
            } else {
                childStartY += childH + 5;
                childrenMaxWidth = std::max(childrenMaxWidth, childW);
                childrenMaxHeight += childH + 5;
            }
        }
    }
    
    outWidth = childrenMaxWidth;
    outHeight = childrenMaxHeight;
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
            // X Resolution
            if (!node.resolvedX) {
                bool startReady = (node.startToStart == 0 || node.startToStart == 0xFFFFFFFF || (getNode(node.startToStart) && getNode(node.startToStart)->resolvedX)) ||
                                  (node.startToEnd == 0 || node.startToEnd == 0xFFFFFFFF || (getNode(node.startToEnd) && getNode(node.startToEnd)->resolvedX));
                bool endReady = (node.endToStart == 0 || node.endToStart == 0xFFFFFFFF || (getNode(node.endToStart) && getNode(node.endToStart)->resolvedX)) ||
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
                        if (hasStart && hasEnd) node.w = std::max(0, endCoord - startCoord);
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
                bool topReady = (node.topToTop == 0 || node.topToTop == 0xFFFFFFFF || (getNode(node.topToTop) && getNode(node.topToTop)->resolvedY)) ||
                                (node.topToBottom == 0 || node.topToBottom == 0xFFFFFFFF || (getNode(node.topToBottom) && getNode(node.topToBottom)->resolvedY));
                bool bottomReady = (node.bottomToTop == 0 || node.bottomToTop == 0xFFFFFFFF || (getNode(node.bottomToTop) && getNode(node.bottomToTop)->resolvedY)) ||
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
                        if (hasTop && hasBottom) node.h = std::max(0, bottomCoord - topCoord);
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
        
        if (node.hwnd) {
            SetWindowPos(node.hwnd, nullptr, node.x, node.y, node.w, node.h, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}
