#pragma once

#include "VectorDrawable.h"
#include "../../animation/ValueAnimator.h"
#include <map>
#include <string>

namespace setu {
namespace graphics {

class AnimatedVectorDrawable : public Drawable {
public:
    AnimatedVectorDrawable(std::shared_ptr<VectorDrawable> vd) : mVectorDrawable(std::move(vd)) {}

    void draw(Canvas& canvas) override {
        if (mVectorDrawable) {
            mVectorDrawable->draw(canvas);
        }
    }

    int getIntrinsicWidth() const override { return mVectorDrawable ? mVectorDrawable->getIntrinsicWidth() : -1; }
    int getIntrinsicHeight() const override { return mVectorDrawable ? mVectorDrawable->getIntrinsicHeight() : -1; }
    
    void setAlpha(int alpha) override {
        if (mVectorDrawable) mVectorDrawable->setAlpha(alpha);
    }
    
    int getAlpha() const override { 
        return mVectorDrawable ? mVectorDrawable->getAlpha() : 255; 
    }

    void addAnimation(const std::string& targetName, std::shared_ptr<animation::ValueAnimator> animator) {
        if (!mVectorDrawable || !animator) return;
        auto target = mVectorDrawable->getTargetByName(targetName);
        if (target) {
            // Need to set target for ObjectAnimators somehow... 
            // Wait, ValueAnimator doesn't have setTarget, ObjectAnimator does.
            // But we don't want to cast if we don't have to... Wait, AnimatorInflater 
            // returns ValueAnimator, but it could be an AnimatorSet containing ObjectAnimators.
            // How does Android set targets for an AnimatorSet? 
            // AnimatorSet::setTarget(Object) sets the target for ALL its child animators.
            // We should add setTarget to ValueAnimator (virtual) or implement it.
            // Or AnimatedVectorDrawable just holds the animators and sets the target.
        }
    }

    void start() override {
        m_running = true;
        for (auto& anim : mAnimators) {
            anim->start();
        }
    }

    void stop() override {
        m_running = false;
        for (auto& anim : mAnimators) {
            anim->cancel();
        }
    }

    bool isRunning() const override { return m_running; }

    void registerAnimator(std::shared_ptr<animation::ValueAnimator> anim) {
        mAnimators.push_back(anim);
        // We add an update listener to trigger redraw!
        anim->addUpdateListener([this](animation::ValueAnimator*) {
            invalidateSelf();
        });
    }

protected:
    void onBoundsChange(const Rect& bounds) override {
        if (mVectorDrawable) mVectorDrawable->setBounds(bounds.left, bounds.top, bounds.right, bounds.bottom);
    }

private:
    std::shared_ptr<VectorDrawable> mVectorDrawable;
    std::vector<std::shared_ptr<animation::ValueAnimator>> mAnimators;
    bool m_running = false;
};

} // namespace graphics
} // namespace setu
