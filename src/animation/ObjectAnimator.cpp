#include "ObjectAnimator.h"
#include "../utils/Logger.h"

namespace setu {
namespace animation {

std::shared_ptr<ObjectAnimator> ObjectAnimator::ofFloat(std::shared_ptr<void> target, const std::string& targetClass, 
                                                        const std::string& propertyName, std::vector<float> values) {
    auto anim = std::make_shared<ObjectAnimator>();
    anim->setTarget(target, targetClass);
    anim->setPropertyName(propertyName);
    anim->setFloatValues(values);
    return anim;
}

void ObjectAnimator::start() {
    m_property = PropertyRegistry::getFloatProperty(m_targetClass, m_propertyName);
    if (!m_property) {
        Logger::w("ObjectAnimator", "Cannot start animator: Property '" + m_propertyName + "' not found for class '" + m_targetClass + "'.");
        return; // Don't run if property is invalid
    }
    ValueAnimator::start();
}

void ObjectAnimator::onAnimationUpdate(float fraction) {
    ValueAnimator::onAnimationUpdate(fraction);
    if (m_property && m_property->setter && m_target) {
        m_property->setter(m_target.get(), getAnimatedValue());
    }
}

}
}
