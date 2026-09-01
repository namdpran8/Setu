#include "Handler.h"
#include "../utils/SystemClock.h"

namespace setu {
namespace os {

std::atomic<uint64_t> Handler::s_tokenCounter{1};

Handler::Handler(Looper* looper) {
    if (looper == nullptr) {
        looper = Looper::myLooper();
    }
    m_looper = looper;
    m_queue = looper ? looper->getQueue() : nullptr;
}

Handler::~Handler() {
    if (m_queue) {
        m_queue->removeCallbacksAndMessages(this, nullptr);
    }
}

Message* Handler::getPostMessage(std::function<void()> r, void* dalvikObj, uint64_t token) {
    Message* m = new Message();
    m->callback = r;
    m->callbackObj = dalvikObj;
    m->token = token;
    m->target = this;
    return m;
}

uint64_t Handler::post(std::function<void()> r) {
    return postDelayed(r, 0);
}

uint64_t Handler::postDelayed(std::function<void()> r, long long delayMillis) {
    uint64_t token = s_tokenCounter.fetch_add(1);
    Message* msg = getPostMessage(r, nullptr, token);
    sendMessageDelayed(msg, delayMillis);
    return token;
}

void Handler::postDelayedWithDalvikObj(std::function<void()> r, long long delayMillis, void* dalvikObj) {
    uint64_t token = s_tokenCounter.fetch_add(1);
    Message* msg = getPostMessage(r, dalvikObj, token);
    sendMessageDelayed(msg, delayMillis);
}

void Handler::removeCallbacks(void* dalvikObj) {
    if (m_queue) {
        m_queue->removeCallbacks(this, dalvikObj);
    }
}

void Handler::removeCallbacksByToken(uint64_t token) {
    if (m_queue) {
        m_queue->removeCallbacksByToken(this, token);
    }
}

void Handler::removeCallbacksAndMessages(void* obj) {
    if (m_queue) {
        m_queue->removeCallbacksAndMessages(this, obj);
    }
}

bool Handler::sendMessage(Message* msg) {
    return sendMessageDelayed(msg, 0);
}

bool Handler::sendMessageDelayed(Message* msg, long long delayMillis) {
    if (delayMillis < 0) {
        delayMillis = 0;
    }
    return sendMessageAtTime(msg, setu::uptimeMillis() + delayMillis);
}

bool Handler::sendMessageAtTime(Message* msg, long long uptimeMillis) {
    if (!m_queue) {
        return false;
    }
    msg->target = this;
    return m_queue->enqueueMessage(msg, uptimeMillis);
}

void Handler::dispatchMessage(Message* msg) {
    if (msg->callback) {
        msg->callback();
    } else {
        handleMessage(msg);
    }
}

void Handler::handleMessage(Message* msg) {
    // Default implementation does nothing
}

} // namespace os
} // namespace setu
