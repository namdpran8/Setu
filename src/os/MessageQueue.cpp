#include "MessageQueue.h"
#include "../ui/WindowManager.h"
#include "../utils/SystemClock.h"

namespace setu {
namespace os {

MessageQueue::MessageQueue() : m_messages(nullptr), m_quitting(false) {}

MessageQueue::~MessageQueue() {
    std::lock_guard<std::mutex> lock(m_mutex);
    Message* p = m_messages;
    while (p != nullptr) {
        Message* n = p->next;
        delete p;
        p = n;
    }
}

bool MessageQueue::enqueueMessage(Message* msg, long long when) {
    if (!msg || !msg->target) return false;
    msg->when = when;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_quitting) {
        delete msg;
        return false;
    }

    Message* p = m_messages;
    bool needWake = false;

    if (p == nullptr || when == 0 || when < p->when) {
        msg->next = p;
        m_messages = msg;
        needWake = true;
    } else {
        Message* prev = nullptr;
        while (p != nullptr && p->when <= when) {
            prev = p;
            p = p->next;
        }
        msg->next = prev->next;
        prev->next = msg;
    }

    if (needWake) {
        long long now = setu::uptimeMillis();
        long long delay = (when <= now) ? 0 : (when - now);
        wake(delay);
    }
    return true;
}

Message* MessageQueue::next() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_quitting) return nullptr;

    long long now = setu::uptimeMillis();
    Message* msg = m_messages;
    
    if (msg != nullptr) {
        if (now >= msg->when) {
            m_messages = msg->next;
            msg->next = nullptr;
            
            if (m_messages) {
                long long delay = (m_messages->when <= now) ? 0 : (m_messages->when - now);
                wake(delay);
            }
            
            return msg;
        } else {
            wake(msg->when - now);
        }
    }
    return nullptr;
}

void MessageQueue::removeCallbacksAndMessages(Handler* h, void* obj) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Message* p = m_messages;
    Message* prev = nullptr;
    
    while (p != nullptr) {
        Message* n = p->next;
        if (p->target == h && (obj == nullptr || p->obj == obj)) {
            if (prev) {
                prev->next = n;
            } else {
                m_messages = n;
            }
            delete p;
        } else {
            prev = p;
        }
        p = n;
    }
}

void MessageQueue::removeCallbacks(Handler* h, void* runnableObj) {
    if (!runnableObj) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Message* p = m_messages;
    Message* prev = nullptr;
    
    while (p != nullptr) {
        Message* n = p->next;
        if (p->target == h && p->callbackObj == runnableObj) {
            if (prev) {
                prev->next = n;
            } else {
                m_messages = n;
            }
            delete p;
        } else {
            prev = p;
        }
        p = n;
    }
}

void MessageQueue::removeCallbacksByToken(Handler* h, uint64_t token) {
    if (token == 0) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    Message* p = m_messages;
    Message* prev = nullptr;
    
    while (p != nullptr) {
        Message* n = p->next;
        if (p->target == h && p->token == token) {
            if (prev) {
                prev->next = n;
            } else {
                m_messages = n;
            }
            delete p;
        } else {
            prev = p;
        }
        p = n;
    }
}

void MessageQueue::quit() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_quitting = true;
    wake(0);
}

void MessageQueue::wake(long long delay) {
    ::WindowManager::wakeLooper(delay);
}

} // namespace os
} // namespace setu
