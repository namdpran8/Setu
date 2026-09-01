#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <string>

namespace setu {
namespace animation {

class ValueAnimator : public std::enable_shared_from_this<ValueAnimator> {
public:
    ValueAnimator();
    virtual ~ValueAnimator();

    static std::shared_ptr<ValueAnimator> ofFloat(std::vector<float> values);
    
    static void doFrame(long long frameTimeNanos);

    virtual void start();
    virtual void cancel();
    virtual void end();

    void setDuration(long long durationMs) { m_duration = durationMs; }
    long long getDuration() const { return m_duration; }
    
    // Values
    void setFloatValues(std::vector<float> values) { m_floatValues = values; }

    // Listeners
    using UpdateListener = std::function<void(ValueAnimator*)>;
    void addUpdateListener(UpdateListener listener) { m_updateListeners.push_back(listener); }

    using AnimatorListener = std::function<void(ValueAnimator*, const std::string& event)>; // "start", "end", "cancel"
    void addListener(AnimatorListener listener) { m_listeners.push_back(listener); }

    float getAnimatedFraction() const { return m_fraction; }
    float getAnimatedValue() const { return m_currentValue; }

protected:
    virtual void onAnimationUpdate(float fraction);
    float evaluate(float fraction, float start, float end);
    void processTick(long long frameTimeNanos);

private:
    static std::vector<std::shared_ptr<ValueAnimator>>& getActiveAnimators();

    long long m_duration = 300;
    long long m_startTime = 0;
    bool m_running = false;
    float m_fraction = 0.0f;
    float m_currentValue = 0.0f;

    std::vector<float> m_floatValues;
    std::vector<UpdateListener> m_updateListeners;
    std::vector<AnimatorListener> m_listeners;
};

} // namespace animation
} // namespace setu
