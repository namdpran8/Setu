#pragma once
#include <windows.h>
#include <vector>
#include "../AxmlPraserer/AxmlParser.h"
#include "../dex/ResourceManager.h"

struct ConstraintNode {
    HWND hwnd = nullptr;
    uint32_t id = 0;
    int width = 0, height = 0; // Intrinsic/requested dimensions
    
    // Width/Height mode: 0 = wrap_content/fixed, 1 = match_parent, 2 = 0dp (match_constraint)
    int widthMode = 0;
    int heightMode = 0;
    
    // Constraints (Target IDs, 0 = parent, 0xFFFFFFFF = none)
    uint32_t topToTop = 0xFFFFFFFF;
    uint32_t topToBottom = 0xFFFFFFFF;
    uint32_t bottomToTop = 0xFFFFFFFF;
    uint32_t bottomToBottom = 0xFFFFFFFF;
    uint32_t startToStart = 0xFFFFFFFF;
    uint32_t startToEnd = 0xFFFFFFFF;
    uint32_t endToStart = 0xFFFFFFFF;
    uint32_t endToEnd = 0xFFFFFFFF;
    
    float horizontalBias = 0.5f;
    float verticalBias = 0.5f;

    // Guideline support
    bool isGuideline = false;
    bool isHorizontalGuide = false;
    float guidePercent = -1.0f;
    int guideBegin = -1;
    int guideEnd = -1;
    
    // Layout types
    bool isConstraintLayout = false;
    bool isLinearLayout = false;
    bool isHorizontal = false;
    
    // Child nodes
    std::vector<ConstraintNode> children;

    // Computed absolute bounds
    int x = 0, y = 0, w = 0, h = 0;
    bool resolvedX = false;
    bool resolvedY = false;
};

class LayoutInflater {
public:
    static void inflate(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager);
    
    // Creates a Win32 control dynamically mapped from an Android class name (e.g., "Landroid/widget/TextView;")
    static HWND createDynamicView(const std::string& className, HWND parentHwnd);

private:
    static ConstraintNode inflateRecursive(const AxmlNode* node, HWND parentHwnd, ResourceManager* resManager, HFONT hFont);
    static std::string resolveString(const AxmlAttribute& attr, ResourceManager* resManager);
    static void resolveConstraints(std::vector<ConstraintNode>& nodes, int parentWidth, int parentHeight);
    static void layoutNode(ConstraintNode& node, int parentWidth, int parentHeight);
};
