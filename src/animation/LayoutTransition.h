#pragma once

#include <memory>

namespace setu {
namespace view { class View; class ViewGroup; }
namespace animation {

class LayoutTransition {
public:
    LayoutTransition() = default;
    ~LayoutTransition() = default;

    void addChild(view::ViewGroup* parent, std::shared_ptr<view::View> child);
    void removeChild(view::ViewGroup* parent, std::shared_ptr<view::View> child);
};

} // namespace animation
} // namespace setu
