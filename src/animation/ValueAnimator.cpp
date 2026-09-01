#include "ValueAnimator.h"
#include "../os/Looper.h"
#include "../os/Handler.h"
#include "../utils/SystemClock.h"
#include <algorithm>

namespace setu {
namespace animation {

ValueAnimator::ValueAnimator() {}
ValueAnimator::~ValueAnimator() { cancel(); }

std::shared_ptr<ValueAnimator> ValueAnimator::ofFloat(std::vector<float> values) {
    auto anim = std::make_shared<ValueAnimator>();
    anim->setFloatValues(values);
    return anim;
}

std::vector<std::shared_ptr<ValueAnimator>>& ValueAnimator::getActiveAnimators() {
    static std::vector<std::shared_ptr<ValueAnimator>> s_animators;
    return s_animators;
}

void ValueAnimator::doFrame(long long frameTimeNanos) {
    auto& animators = getActiveAnimators();
    if (animators.empty()) return;
    
    std::vector<std::shared_ptr<ValueAnimator>> currentAnimators = animators;
    for (auto& anim : currentAnimators) {
        if (anim->m_running) {
            anim->processTick(frameTimeNanos);
        }
    }
}

void ValueAnimator::start() {
    if (m_running) return;
    if (m_floatValues.size() < 2) return;
    
    m_running = true;
    m_startTime = setu::uptimeMillis();
    m_fraction = 0.0f;
    m_currentValue = m_floatValues[0];
    
    for (auto& l : m_listeners) l(this, "start");
    
    getActiveAnimators().push_back(shared_from_this());
}

void ValueAnimator::processTick(long long frameTimeNanos) {
    if (!m_running) return;
    
    long long now = setu::uptimeMillis(); // Eventually sync with frameTimeNanos
    long long elapsed = now - m_startTime;
    
    if (elapsed >= m_duration) {
        m_fraction = 1.0f;
        onAnimationUpdate(m_fraction);
        m_running = false;
        
        auto& animators = getActiveAnimators();
        animators.erase(std::remove(animators.begin(), animators.end(), shared_from_this()), animators.end());
        
        for (auto& l : m_listeners) l(this, "end");
    } else {
        m_fraction = (float)elapsed / m_duration;
        onAnimationUpdate(m_fraction);
    }
}

void ValueAnimator::onAnimationUpdate(float fraction) {
    if (m_floatValues.size() >= 2) {
        m_currentValue = evaluate(fraction, m_floatValues[0], m_floatValues[1]);
    }
    for (auto& l : m_updateListeners) {
        l(this);
    }
}

float ValueAnimator::evaluate(float fraction, float start, float end) {
    return start + fraction * (end - start);
}

void ValueAnimator::cancel() {
    if (m_running) {
        m_running = false;
        auto& animators = getActiveAnimators();
        animators.erase(std::remove(animators.begin(), animators.end(), shared_from_this()), animators.end());
        for (auto& l : m_listeners) l(this, "cancel");
    }
}

void ValueAnimator::end() {
    if (m_running) {
        m_fraction = 1.0f;
        onAnimationUpdate(m_fraction);
        m_running = false;
        auto& animators = getActiveAnimators();
        animators.erase(std::remove(animators.begin(), animators.end(), shared_from_this()), animators.end());
        for (auto& l : m_listeners) l(this, "end");
    }
}

}
}
