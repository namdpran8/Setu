#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "../AxmlPraserer/AxmlParser.h"
#include "../dex/ResourceManager.h"
#include "../view/View.h"

struct ConstraintNode {
    std::shared_ptr<windroid::view::View> view = nullptr;
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
    static std::shared_ptr<windroid::view::View> inflate(const AxmlNode* node, ResourceManager* resManager, int parentWidth, int parentHeight);
    
private:
    static ConstraintNode inflateRecursive(const AxmlNode* node, ResourceManager* resManager);
    static std::string resolveString(const AxmlAttribute& attr, ResourceManager* resManager);
    static void resolveConstraints(std::vector<ConstraintNode>& nodes, int parentWidth, int parentHeight);
    static void layoutNode(ConstraintNode& node, int parentWidth, int parentHeight);
};
