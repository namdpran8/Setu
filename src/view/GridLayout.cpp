#include "androidfw/Util.h"
#include "GridLayout.h"
#include <algorithm>
#include <cmath>
#include "androidfw/ResourceTypes.h"

namespace windroid {
namespace view {

GridLayout::GridLayout() {}

void GridLayout::setRowCount(int count) { mRowCount = count; }
void GridLayout::setColumnCount(int count) { mColumnCount = count; }

void GridLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Step 1: Assign Implicit Indices
    int cursorCol = 0;
    int cursorRow = 0;
    int maxCol = mColumnCount > 0 ? mColumnCount : 1;
    int maxRow = mRowCount > 0 ? mRowCount : 1;

    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        if (lp->columnSpec.spanStart == -1 && lp->rowSpec.spanStart == -1) {
            lp->columnSpec.spanStart = cursorCol;
            lp->rowSpec.spanStart = cursorRow;

            cursorCol += lp->columnSpec.spanSize;
            if (mColumnCount > 0 && cursorCol >= mColumnCount) {
                cursorCol = 0;
                cursorRow++;
            }
        }
        
        maxCol = std::max(maxCol, lp->columnSpec.spanStart + lp->columnSpec.spanSize);
        maxRow = std::max(maxRow, lp->rowSpec.spanStart + lp->rowSpec.spanSize);
    }

    if (mColumnCount == 0) mColumnCount = maxCol;
    if (mRowCount == 0) mRowCount = maxRow;

    // Step 2: Measure Children (Pass 1 - Unconstrained)
    for (auto& child : mChildren) {
        int childWidthSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
        int childHeightSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
        child->measure(childWidthSpec, childHeightSpec);
    }

    // Step 3: Solve Column Widths
    std::vector<int> columnWidths(mColumnCount, 0);
    std::vector<int> rowHeights(mRowCount, 0);
    std::vector<float> columnWeights(mColumnCount, 0.0f);
    std::vector<float> rowWeights(mRowCount, 0.0f);

    // Single spans
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        if (lp->columnSpec.spanSize == 1) {
            int c = lp->columnSpec.spanStart;
            if (c >= 0 && c < mColumnCount) {
                columnWidths[c] = std::max(columnWidths[c], child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
                columnWeights[c] = std::max(columnWeights[c], lp->columnSpec.weight);
            }
        }
        if (lp->rowSpec.spanSize == 1) {
            int r = lp->rowSpec.spanStart;
            if (r >= 0 && r < mRowCount) {
                rowHeights[r] = std::max(rowHeights[r], child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
                rowWeights[r] = std::max(rowWeights[r], lp->rowSpec.weight);
            }
        }
    }

    // Multiple spans
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        if (lp->columnSpec.spanSize > 1) {
            int start = lp->columnSpec.spanStart;
            int size = lp->columnSpec.spanSize;
            int currentSpanWidth = 0;
            for (int i = 0; i < size; ++i) {
                if (start + i < mColumnCount) currentSpanWidth += columnWidths[start + i];
            }
            int needed = child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
            if (needed > currentSpanWidth) {
                int extra = (needed - currentSpanWidth) / size;
                for (int i = 0; i < size; ++i) {
                    if (start + i < mColumnCount) columnWidths[start + i] += extra;
                }
            }
        }
        if (lp->rowSpec.spanSize > 1) {
            int start = lp->rowSpec.spanStart;
            int size = lp->rowSpec.spanSize;
            int currentSpanHeight = 0;
            for (int i = 0; i < size; ++i) {
                if (start + i < mRowCount) currentSpanHeight += rowHeights[start + i];
            }
            int needed = child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
            if (needed > currentSpanHeight) {
                int extra = (needed - currentSpanHeight) / size;
                for (int i = 0; i < size; ++i) {
                    if (start + i < mRowCount) rowHeights[start + i] += extra;
                }
            }
        }
    }

    // Step 4: Distribute Weights
    int totalColWidth = 0;
    float totalColWeight = 0.0f;
    for (int i = 0; i < mColumnCount; ++i) {
        totalColWidth += columnWidths[i];
        totalColWeight += columnWeights[i];
    }
    if (widthMode == View::MEASURE_SPEC_EXACTLY && totalColWidth < widthSize && totalColWeight > 0.0f) {
        int extraWidth = widthSize - totalColWidth;
        for (int i = 0; i < mColumnCount; ++i) {
            if (columnWeights[i] > 0.0f) {
                columnWidths[i] += (int)(extraWidth * (columnWeights[i] / totalColWeight));
            }
        }
    }

    int totalRowHeight = 0;
    float totalRowWeight = 0.0f;
    for (int i = 0; i < mRowCount; ++i) {
        totalRowHeight += rowHeights[i];
        totalRowWeight += rowWeights[i];
    }
    if (heightMode == View::MEASURE_SPEC_EXACTLY && totalRowHeight < heightSize && totalRowWeight > 0.0f) {
        int extraHeight = heightSize - totalRowHeight;
        for (int i = 0; i < mRowCount; ++i) {
            if (rowWeights[i] > 0.0f) {
                rowHeights[i] += (int)(extraHeight * (rowWeights[i] / totalRowWeight));
            }
        }
    }

    // Build Grid Lines
    mHorizontalGridLines.assign(mColumnCount + 1, 0);
    int currentX = 0;
    for (int i = 0; i < mColumnCount; ++i) {
        mHorizontalGridLines[i] = currentX;
        currentX += columnWidths[i];
    }
    mHorizontalGridLines[mColumnCount] = currentX;

    mVerticalGridLines.assign(mRowCount + 1, 0);
    int currentY = 0;
    for (int i = 0; i < mRowCount; ++i) {
        mVerticalGridLines[i] = currentY;
        currentY += rowHeights[i];
    }
    mVerticalGridLines[mRowCount] = currentY;

    // Step 5: Measure Children (Pass 2 - Exact)
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        int col = lp->columnSpec.spanStart;
        int colSpan = lp->columnSpec.spanSize;
        int row = lp->rowSpec.spanStart;
        int rowSpan = lp->rowSpec.spanSize;

        if (col < 0) col = 0;
        if (col + colSpan > mColumnCount) colSpan = mColumnCount - col;
        if (row < 0) row = 0;
        if (row + rowSpan > mRowCount) rowSpan = mRowCount - row;

        int cellWidth = mHorizontalGridLines[col + colSpan] - mHorizontalGridLines[col];
        int cellHeight = mVerticalGridLines[row + rowSpan] - mVerticalGridLines[row];

        cellWidth -= (lp->leftMargin + lp->rightMargin);
        cellHeight -= (lp->topMargin + lp->bottomMargin);
        if (cellWidth < 0) cellWidth = 0;
        if (cellHeight < 0) cellHeight = 0;

        int exactWidthSpec = View::makeMeasureSpec(cellWidth, 
            (lp->columnSpec.alignment == FILL || lp->width == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
        int exactHeightSpec = View::makeMeasureSpec(cellHeight, 
            (lp->rowSpec.alignment == FILL || lp->height == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);

        child->measure(exactWidthSpec, exactHeightSpec);
    }

    // Step 6: Set Final Dimension
    int finalWidth = currentX; // total sum of column widths
    int finalHeight = currentY; // total sum of row heights

    if (widthMode == View::MEASURE_SPEC_EXACTLY) finalWidth = widthSize;
    if (heightMode == View::MEASURE_SPEC_EXACTLY) finalHeight = heightSize;

    setMeasuredDimension(finalWidth, finalHeight);
}

void GridLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int paddingLeft = 0;
    int paddingTop = 0;

    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        int col = lp->columnSpec.spanStart;
        int colSpan = lp->columnSpec.spanSize;
        int row = lp->rowSpec.spanStart;
        int rowSpan = lp->rowSpec.spanSize;

        if (col < 0) col = 0;
        if (col + colSpan > mColumnCount) colSpan = mColumnCount - col;
        if (row < 0) row = 0;
        if (row + rowSpan > mRowCount) rowSpan = mRowCount - row;

        int cellLeft = paddingLeft + mHorizontalGridLines[col];
        int cellRight = paddingLeft + mHorizontalGridLines[col + colSpan];
        
        int cellTop = paddingTop + mVerticalGridLines[row];
        int cellBottom = paddingTop + mVerticalGridLines[row + rowSpan];

        cellLeft += lp->leftMargin;
        cellRight -= lp->rightMargin;
        cellTop += lp->topMargin;
        cellBottom -= lp->bottomMargin;

        int childWidth = child->getMeasuredWidth();
        int childHeight = child->getMeasuredHeight();

        int finalLeft = cellLeft;
        int finalTop = cellTop;

        if (lp->columnSpec.alignment == CENTER) {
            finalLeft = cellLeft + ((cellRight - cellLeft) - childWidth) / 2;
        } else if (lp->columnSpec.alignment == END) {
            finalLeft = cellRight - childWidth;
        } // START and FILL default to cellLeft/cellTop

        if (lp->rowSpec.alignment == CENTER) {
            finalTop = cellTop + ((cellBottom - cellTop) - childHeight) / 2;
        } else if (lp->rowSpec.alignment == END) {
            finalTop = cellBottom - childHeight;
        }

        child->layout(finalLeft, finalTop, finalLeft + childWidth, finalTop + childHeight);
    }
}

std::shared_ptr<View::LayoutParams> GridLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
    ViewGroup::parseBaseLayoutParams(lp, parser);
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

        if (attrName == "layout_column") lp->columnSpec.spanStart = data;
        else if (attrName == "layout_row") lp->rowSpec.spanStart = data;
        else if (attrName == "layout_columnSpan") lp->columnSpec.spanSize = data;
        else if (attrName == "layout_rowSpan") lp->rowSpec.spanSize = data;
        else if (attrName == "layout_columnWeight") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->columnSpec.weight = (type == 0x04) ? u.f : (float)data;
        }
        else if (attrName == "layout_rowWeight") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->rowSpec.weight = (type == 0x04) ? u.f : (float)data;
        }
        else if (attrName == "layout_gravity") {
            int gravity = data;
            if ((gravity & 0x7) == 0x7) lp->columnSpec.alignment = GridLayout::FILL;
            else if ((gravity & 0x1) == 0x1) lp->columnSpec.alignment = GridLayout::CENTER;
            else if ((gravity & 0x5) == 0x5 || (gravity & 0x800005) == 0x800005) lp->columnSpec.alignment = GridLayout::END;
            else lp->columnSpec.alignment = GridLayout::START;
            
            if ((gravity & 0x70) == 0x70) lp->rowSpec.alignment = GridLayout::FILL;
            else if ((gravity & 0x10) == 0x10) lp->rowSpec.alignment = GridLayout::CENTER;
            else if ((gravity & 0x50) == 0x50 || (gravity & 0x800050) == 0x800050) lp->rowSpec.alignment = GridLayout::END;
            else lp->rowSpec.alignment = GridLayout::START;
        }
    }
    return lp;
}

} // namespace view
} // namespace windroid




