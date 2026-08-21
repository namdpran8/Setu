#pragma once

#include "ConstraintWidget.h"
#include <vector>
#include <memory>

namespace setu::cassowary {

class Cache;

class WidgetContainer : public ConstraintWidget {
public:
    std::vector<std::unique_ptr<ConstraintWidget>> mChildren;

    WidgetContainer();
    WidgetContainer(int x, int y, int width, int height);
    WidgetContainer(int width, int height);
    virtual ~WidgetContainer() = default;

    virtual void reset() override;
    
    void add(std::unique_ptr<ConstraintWidget> widget);
    std::unique_ptr<ConstraintWidget> remove(ConstraintWidget* widget);
    
    const std::vector<std::unique_ptr<ConstraintWidget>>& getChildren() const;
    
    class ConstraintWidgetContainer* getRootConstraintContainer();
    
    virtual void setOffset(int x, int y) override;
    
    virtual void layout();
    
    virtual void resetSolverVariables(Cache* cache) override;
    
    void removeAllChildren();
};

} // namespace setu::cassowary
