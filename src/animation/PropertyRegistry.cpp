#include "../graphics/drawable/VectorDrawable.h"
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
    // VPath properties
    registerFloatProperty("VPath", "trimPathEnd",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->trimPathEnd = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->trimPathEnd; }
    );
    registerFloatProperty("VPath", "trimPathStart",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->trimPathStart = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->trimPathStart; }
    );
    registerFloatProperty("VPath", "trimPathOffset",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->trimPathOffset = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->trimPathOffset; }
    );
    registerFloatProperty("VPath", "fillAlpha",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->fillAlpha = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->fillAlpha; }
    );
    registerFloatProperty("VPath", "strokeAlpha",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->strokeAlpha = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->strokeAlpha; }
    );
    registerFloatProperty("VPath", "strokeWidth",
        [](void* target, float v) { static_cast<graphics::VPath*>(target)->strokeWidth = v; },
        [](void* target) -> float { return static_cast<graphics::VPath*>(target)->strokeWidth; }
    );

    // VGroup properties
    registerFloatProperty("VGroup", "rotation",
        [](void* target, float v) { static_cast<graphics::VGroup*>(target)->rotation = v; },
        [](void* target) -> float { return static_cast<graphics::VGroup*>(target)->rotation; }
    );
    registerFloatProperty("VGroup", "scaleX",
        [](void* target, float v) { static_cast<graphics::VGroup*>(target)->scaleX = v; },
        [](void* target) -> float { return static_cast<graphics::VGroup*>(target)->scaleX; }
    );
    registerFloatProperty("VGroup", "scaleY",
        [](void* target, float v) { static_cast<graphics::VGroup*>(target)->scaleY = v; },
        [](void* target) -> float { return static_cast<graphics::VGroup*>(target)->scaleY; }
    );
    registerFloatProperty("VGroup", "translateX",
        [](void* target, float v) { static_cast<graphics::VGroup*>(target)->translateX = v; },
        [](void* target) -> float { return static_cast<graphics::VGroup*>(target)->translateX; }
    );
    registerFloatProperty("VGroup", "translateY",
        [](void* target, float v) { static_cast<graphics::VGroup*>(target)->translateY = v; },
        [](void* target) -> float { return static_cast<graphics::VGroup*>(target)->translateY; }
    );
}

}
}
