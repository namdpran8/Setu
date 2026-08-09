#include "LayoutInflater.h"
#include "../utils/Logger.h"

void LayoutInflater::inflate(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager) {
    int currentY = 50; // Start with a 50px top margin
    inflateRecursive(node, parentHwnd, resManager, currentY);
}

HWND LayoutInflater::createDynamicView(const std::string& className, HWND parentHwnd) {
    std::string win32Class = "";
    DWORD style = WS_CHILD | WS_VISIBLE; // Note: Child windows MUST have a parent
    DWORD exStyle = 0;
    
    if (className.find("TextView") != std::string::npos) {
        win32Class = "STATIC";
    } else if (className.find("Button") != std::string::npos) {
        win32Class = "BUTTON";
    } else if (className.find("EditText") != std::string::npos) {
        win32Class = "EDIT";
        style |= WS_BORDER;
        exStyle |= WS_EX_CLIENTEDGE;
    } else if (className.find("HorizontalScrollView") != std::string::npos ||
               className.find("ScrollView") != std::string::npos) {
        // Simple container mapping
        win32Class = "STATIC"; 
    } else if (className.find("ImageView") != std::string::npos) {
        win32Class = "STATIC";
    } else {
        win32Class = "STATIC"; // Default fallback
    }

    // Give it a generic starting bounds, layout will reposition it later via setLayoutParams/addView
    HWND hwnd = CreateWindowExA(
        exStyle,
        win32Class.c_str(),
        "",
        style,
        0, 0, 100, 100, 
        parentHwnd, // A valid HWND is usually required for WS_CHILD
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    if (hwnd) {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
    
    return hwnd;
}

std::string LayoutInflater::resolveString(const AxmlAttribute& attr, ResourceManager* resManager) {
    if (!attr.rawValue.empty()) {
        return attr.rawValue; // Raw string literal
    }
    
    // Type 0x01 is TYPE_REFERENCE, Type 0x03 is TYPE_STRING
    if (attr.typedValueType == 0x01 || attr.typedValueType == 0x03) {
        if (resManager) {
            std::string resolved = resManager->getResourcePath(attr.typedValueData);
            if (!resolved.empty()) {
                return resolved;
            }
        }
    }
    return "";
}

void LayoutInflater::inflateRecursive(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager, int& currentY) {
    if (!node) return;
    
    std::string tag = node->tag;
    
    // Default mapped Win32 class
    std::string win32Class = "";
    DWORD style = WS_CHILD | WS_VISIBLE;
    DWORD exStyle = 0;
    
    if (tag.find("TextView") != std::string::npos) {
        win32Class = "STATIC";
    } else if (tag.find("Button") != std::string::npos) {
        win32Class = "BUTTON";
    } else if (tag.find("EditText") != std::string::npos) {
        win32Class = "EDIT";
        style |= WS_BORDER;
        exStyle |= WS_EX_CLIENTEDGE;
    } else if (tag.find("ImageView") != std::string::npos) {
        win32Class = "STATIC";
    }

    std::string text = "";
    uint32_t id = 0;
    
    // Parse attributes
    for (const auto& attr : node->attributes) {
        if (attr.name == "text") {
            text = resolveString(attr, resManager);
        } else if (attr.name == "id") {
            if (attr.typedValueType == 0x01) { // TYPE_REFERENCE
                id = attr.typedValueData;
            }
        }
    }

    bool isDummy = false;
    if (win32Class.empty()) {
        win32Class = "STATIC";
        isDummy = true;
    }
    
    // Instantiate it
    if (!win32Class.empty()) {
        int width = isDummy ? 0 : 300;
        int height = isDummy ? 0 : 50;
        
        HWND hwnd = CreateWindowExA(
            exStyle,
            win32Class.c_str(),
            text.c_str(),
            style,
            50, currentY, width, height, // Fixed X at 50, dynamic Y
            parentHwnd,
            (HMENU)(INT_PTR)id, // Explicit double-cast for 64-bit safety
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (hwnd) {
            // Set default font to make it look native
            SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        }
        
        // Advance Y for the next control if it's visible
        if (!isDummy) {
            currentY += height + 20; // 20px padding
        }
    }
    
    // Recurse into children (flattening containers naturally)
    for (const auto& child : node->children) {
        inflateRecursive(child.get(), parentHwnd, resManager, currentY);
    }
}
