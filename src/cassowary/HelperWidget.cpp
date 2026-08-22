#include "HelperWidget.h"
#include <algorithm>

namespace setu::cassowary {

void HelperWidget::add(ConstraintWidget* widget) {
    if (widget != this && std::find(mWidgets.begin(), mWidgets.end(), widget) == mWidgets.end()) {
        mWidgets.push_back(widget);
    }
}

void HelperWidget::removeAllIds() {
    mWidgets.clear();
}

void HelperWidget::copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map) {
    ConstraintWidget::copy(src, map);
    if (auto srcHelper = dynamic_cast<HelperWidget*>(src)) {
        mWidgets.clear();
        for (ConstraintWidget* w : srcHelper->mWidgets) {
            auto it = map.find(w);
            if (it != map.end()) {
                mWidgets.push_back(it->second);
            }
        }
    }
}

} // namespace setu::cassowary
