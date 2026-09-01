#pragma once

#include "ValueAnimator.h"
#include "PropertyRegistry.h"
#include <string>

namespace setu {
namespace animation {

class ObjectAnimator : public ValueAnimator {
public:
    ObjectAnimator() = default;

    static std::shared_ptr<ObjectAnimator> ofFloat(std::shared_ptr<void> target, const std::string& targetClass, 
                                                   const std::string& propertyName, std::vector<float> values);

    void setTarget(std::shared_ptr<void> target, const std::string& targetClass) {
        m_target = target;
        m_targetClass = targetClass;
    }
    void setPropertyName(const std::string& propertyName) { m_propertyName = propertyName; }

    void start() override;

protected:
    void onAnimationUpdate(float fraction) override;

private:
    std::shared_ptr<void> m_target = nullptr;
    std::string m_targetClass;
    std::string m_propertyName;
    const FloatProperty* m_property = nullptr;
};

} // namespace animation
} // namespace setu
