#include "PropertyRegistry.h"
#include "../view/View.h"

namespace setu {
namespace animation {

void PropertyRegistry::init() {
    registerFloatProperty("View", "alpha",
        [](void* target, float v) { static_cast<view::View*>(target)->setAlpha(v); },
        [](void* target) -> float { return static_cast<view::View*>(target)->getAlpha(); }
    );
    registerFloatProperty("View", "translationX",
        [](void* target, float v) { static_cast<view::View*>(target)->setTranslationX(v); },
        [](void* target) -> float { return static_cast<view::View*>(target)->getTranslationX(); }
    );
    registerFloatProperty("View", "translationY",
        [](void* target, float v) { static_cast<view::View*>(target)->setTranslationY(v); },
        [](void* target) -> float { return static_cast<view::View*>(target)->getTranslationY(); }
    );
    registerFloatProperty("View", "scaleX",
        [](void* target, float v) { static_cast<view::View*>(target)->setScaleX(v); },
        [](void* target) -> float { return static_cast<view::View*>(target)->getScaleX(); }
    );
    registerFloatProperty("View", "scaleY",
        [](void* target, float v) { static_cast<view::View*>(target)->setScaleY(v); },
        [](void* target) -> float { return static_cast<view::View*>(target)->getScaleY(); }
    );
}

}
}
