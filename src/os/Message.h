#pragma once

#include <functional>
#include <cstdint>

namespace setu {
namespace os {

class Handler;

struct Message {
    int what = 0;
    int arg1 = 0;
    int arg2 = 0;
    void* obj = nullptr;
    long long when = 0;
    Handler* target = nullptr;
    
    std::function<void()> callback = nullptr;
    void* callbackObj = nullptr; // For Dalvik Runnable object matching
    uint64_t token = 0;          // For C++ matching

    // Pointer for linked list in MessageQueue
    Message* next = nullptr;
};

} // namespace os
} // namespace setu
