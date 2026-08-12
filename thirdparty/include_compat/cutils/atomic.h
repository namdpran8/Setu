#pragma once
#include <atomic>
#include <stdint.h>

inline int32_t android_atomic_inc(volatile int32_t* addr) {
    auto a = reinterpret_cast<volatile std::atomic<int32_t>*>(addr);
    return std::atomic_fetch_add(a, 1);
}

inline int32_t android_atomic_dec(volatile int32_t* addr) {
    auto a = reinterpret_cast<volatile std::atomic<int32_t>*>(addr);
    return std::atomic_fetch_sub(a, 1);
}
