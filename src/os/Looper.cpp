#include "Looper.h"
#include "Handler.h"
#include <cassert>

namespace setu {
namespace os {

Looper* Looper::s_mainLooper = nullptr;

Looper::Looper() {}
Looper::~Looper() {}

void Looper::prepareMainLooper() {
    assert(s_mainLooper == nullptr && "Main looper already prepared");
    s_mainLooper = new Looper();
}

Looper* Looper::getMainLooper() {
    return s_mainLooper;
}

Looper* Looper::myLooper() {
    // For this pass, we only support the main thread looper.
    return s_mainLooper;
}

MessageQueue* Looper::getQueue() {
    return &m_queue;
}

bool Looper::loopOnce() {
    Message* msg = m_queue.next();
    if (msg != nullptr) {
        if (msg->target) {
            msg->target->dispatchMessage(msg);
        }
        delete msg;
        return true;
    }
    return false;
}

} // namespace os
} // namespace setu
