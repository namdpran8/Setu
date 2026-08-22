#pragma once

#include "ConstraintWidget.h"
#include <vector>

namespace setu::cassowary {

class HelperWidget : public ConstraintWidget {
public:
    std::vector<ConstraintWidget*> mWidgets;

    HelperWidget() = default;
    virtual ~HelperWidget() = default;

    virtual void add(ConstraintWidget* widget);
    virtual void removeAllIds();
    
    void copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map);
};

} // namespace setu::cassowary
