#pragma once
#include "ValueAnimator.h"

namespace setu {
namespace animation {

class AnimatorSet : public ValueAnimator {
public:
    AnimatorSet() = default;

    void playTogether(std::vector<std::shared_ptr<ValueAnimator>> anims) {
        m_childAnimators = anims;
    }

    void setTarget(std::shared_ptr<void> target) override {
        for (auto& anim : m_childAnimators) anim->setTarget(target);
    }

    void start() override {
        long long maxDuration = 0;
        for (auto& anim : m_childAnimators) {
            if (anim->getDuration() > maxDuration) {
                maxDuration = anim->getDuration();
            }
        }
        setDuration(maxDuration);
        
        ValueAnimator::start();
        
        for (auto& anim : m_childAnimators) {
            anim->start();
        }
    }

    void cancel() override {
        ValueAnimator::cancel();
        for (auto& anim : m_childAnimators) {
            anim->cancel();
        }
    }

    void end() override {
        ValueAnimator::end();
        for (auto& anim : m_childAnimators) {
            anim->end();
        }
    }

private:
    std::vector<std::shared_ptr<ValueAnimator>> m_childAnimators;
};

} // namespace animation
} // namespace setu
