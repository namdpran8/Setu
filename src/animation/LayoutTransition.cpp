#include "LayoutTransition.h"
#include "ObjectAnimator.h"
#include "../view/View.h"
#include "../view/ViewGroup.h"

namespace setu {
namespace animation {

void LayoutTransition::addChild(view::ViewGroup* parent, std::shared_ptr<view::View> child) {
    if (!child) return;
    child->setAlpha(0.0f);
    auto anim = ObjectAnimator::ofFloat(child, "View", "alpha", {0.0f, 1.0f});
    anim->setDuration(300);
    anim->start();
}

void LayoutTransition::removeChild(view::ViewGroup* parent, std::shared_ptr<view::View> child) {
    if (!child) return;
    auto anim = ObjectAnimator::ofFloat(child, "View", "alpha", {child->getAlpha(), 0.0f});
    anim->setDuration(300);
    
    // Add end listener to finish removeView
    anim->addListener([parent, child](ValueAnimator* anim, const std::string& event) {
        if (event == "end" || event == "cancel") {
            parent->finishRemoveView(child);
        }
    });
    
    anim->start();
}

}
}
