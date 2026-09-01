#pragma once

#include "MessageQueue.h"

namespace setu {
namespace os {

class Looper {
public:
    static void prepareMainLooper();
    static Looper* getMainLooper();
    static Looper* myLooper();

    MessageQueue* getQueue();
    
    // Pump one message from the queue. Returns true if a message was processed, false if idle or quitting.
    bool loopOnce();

private:
    Looper();
    ~Looper();

    MessageQueue m_queue;
    static Looper* s_mainLooper;
};

} // namespace os
} // namespace setu
