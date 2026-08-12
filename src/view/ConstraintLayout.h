#pragma once
#include "ViewGroup.h"
#include <map>
#include <string>
#include <vector>

namespace windroid {
namespace view {

class ConstraintLayout : public ViewGroup {
public:
    class LayoutParams : public View::LayoutParams {
    public:
        // Relationships to other view IDs or parent ("parent" typically mapped to 0)
        int topToTop = -1;
        int topToBottom = -1;
        int bottomToTop = -1;
        int bottomToBottom = -1;
        int startToStart = -1;
        int startToEnd = -1;
        int endToStart = -1;
        int endToEnd = -1;

        // Biases
        float horizontalBias = 0.5f;
        float verticalBias = 0.5f;

        // Guidelines
        bool isGuideline = false;
        int guideBegin = -1;
        int guideEnd = -1;
        float guidePercent = -1.0f;
        int orientation = 0; // 0 = horizontal, 1 = vertical

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
    };

    ConstraintLayout() = default;
    virtual ~ConstraintLayout() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;

private:
    struct ResolvedNode {
        std::shared_ptr<View> view;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        bool xResolved = false;
        bool yResolved = false;
        bool wResolved = false;
        bool hResolved = false;
    };

    std::vector<ResolvedNode> mResolvedNodes;
    std::map<int, size_t> mIdToIndex;
    
    void resolveConstraints(int parentWidth, int parentHeight);
    int resolveX(int id, int parentWidth, std::vector<int>& path);
    int resolveY(int id, int parentHeight, std::vector<int>& path);
};

} // namespace view
} // namespace windroid

