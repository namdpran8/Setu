#pragma once

#include <chrono>

namespace setu {

// android.os.SystemClock.uptimeMillis(): milliseconds since an arbitrary but
// fixed origin, monotonic and unaffected by the wall clock being changed.
//
// This is the time base for every animation in the runtime. Animations are
// computed from timestamps rather than counted in frames, so a dropped or late
// frame shifts nothing: a ripple that is 225ms old is fully entered whether that
// took fourteen frames or four.
//
// Header-only on purpose. View.cpp is compiled into the standalone
// constraint_layout_test target as well as the runtime, so anything it reaches
// for has to be either already in that target's source list or inline here.
inline long long uptimeMillis() {
    using clock = std::chrono::steady_clock;
    // One shared origin across every translation unit, so two timestamps taken in
    // different files are comparable. steady_clock's own epoch is unspecified, and
    // subtracting from a fixed point keeps the numbers small enough to read in a log.
    static const clock::time_point origin = clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - origin)
        .count();
}

} // namespace setu
