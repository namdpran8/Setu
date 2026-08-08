#pragma once
#include <windows.h>
#include "../AxmlPraserer/AxmlParser.h"
#include "../dex/ResourceManager.h"

class LayoutInflater {
public:
    static void inflate(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager);

private:
    static void inflateRecursive(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager, int& currentY);
    static std::string resolveString(const AxmlAttribute& attr, ResourceManager* resManager);
};
