#pragma once
#include <windows.h>
#include "../AxmlPraserer/AxmlParser.h"
#include "../dex/ResourceManager.h"

class LayoutInflater {
public:
    static void inflate(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager);
    
    // Creates a Win32 control dynamically mapped from an Android class name (e.g., "Landroid/widget/TextView;")
    static HWND createDynamicView(const std::string& className, HWND parentHwnd);

private:
    static void inflateRecursive(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager, int& currentY);
    static std::string resolveString(const AxmlAttribute& attr, ResourceManager* resManager);
};
