#pragma once

#include <cmath>

namespace setu {

// The easing curves from android.view.animation, as plain functions.
//
// Every one maps a normalised elapsed fraction (0 at the start of the animation,
// 1 at the end) to a normalised progress fraction. Nothing here holds state, so
// an animation driven from uptimeMillis() can ask for its value at any moment
// without a per-frame step - which is the whole reason the runtime animates from
// timestamps rather than counting frames.
//
// Header-only and dependency-free on purpose, like SystemClock.h next door.
namespace interpolator {

inline float clamp01(float value) {
    return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

inline float lerp(float start, float stop, float amount) {
    return start + (stop - start) * amount;
}

// One axis of a cubic Bezier whose endpoints are pinned at 0 and 1, which is the
// only kind android.view.animation.PathInterpolator's two-control-point form can
// describe.
inline float cubicBezier(float p1, float p2, float t) {
    const float mt = 1.0f - t;
    return 3.0f * mt * mt * t * p1 + 3.0f * mt * t * t * p2 + t * t * t;
}

inline float cubicBezierSlope(float p1, float p2, float t) {
    const float mt = 1.0f - t;
    return 3.0f * mt * mt * p1 + 6.0f * mt * t * (p2 - p1) + 3.0f * t * t * (1.0f - p2);
}

// android.view.animation.PathInterpolator(x1, y1, x2, y2).
//
// The curve is parametric, so reading a y off it means first recovering the
// parameter t at which the x-axis curve equals the input. AOSP does that by
// flattening the path into a few thousand line segments at construction and
// linearly interpolating within the bucket the input lands in; Newton's method on
// the x-cubic is both cheaper and more accurate, and needs no precomputation, so
// there is nothing to build and nothing to cache.
//
// Requires x1 and x2 in [0, 1], which is what makes x(t) monotonic and the root
// unique. Every named curve below satisfies that, as does every PathInterpolator
// AAPT will compile.
inline float pathInterpolate(float x1, float y1, float x2, float y2, float input) {
    const float x = clamp01(input);
    if (x <= 0.0f) return 0.0f;
    if (x >= 1.0f) return 1.0f;

    // x(t) is close to t for any reasonable control points, so the input itself is
    // a good enough seed to converge in a handful of steps.
    float t = x;
    for (int i = 0; i < 8; ++i) {
        const float error = cubicBezier(x1, x2, t) - x;
        if (std::fabs(error) < 1e-5f) break;

        const float slope = cubicBezierSlope(x1, x2, t);
        if (std::fabs(slope) < 1e-6f) {
            // A flat spot would send Newton off to infinity. Monotonic x means the
            // root is still bracketed, so fall back to bisection for the rest.
            float low = 0.0f, high = 1.0f;
            for (int j = 0; j < 24; ++j) {
                t = (low + high) * 0.5f;
                if (cubicBezier(x1, x2, t) < x) {
                    low = t;
                } else {
                    high = t;
                }
            }
            break;
        }
        t -= error / slope;
        t = clamp01(t);
    }
    return cubicBezier(y1, y2, t);
}

// android.view.animation.LinearInterpolator. Spelled out rather than left
// implicit, because "no easing" is a deliberate choice for a fade - a linear
// opacity ramp is what makes a ripple's fade-in read as instant rather than soft.
inline float linear(float input) { return clamp01(input); }

// @android:interpolator/fast_out_slow_in - Material's standard easing, and the
// curve every ripple expands along. RippleForeground names its own copy
// DECELERATE_INTERPOLATOR and builds it as PathInterpolator(0.4, 0, 0.2, 1)
// because it has no Context to load the resource with; these are those numbers.
inline float fastOutSlowIn(float input) {
    return pathInterpolate(0.4f, 0.0f, 0.2f, 1.0f, input);
}

} // namespace interpolator
} // namespace setu
