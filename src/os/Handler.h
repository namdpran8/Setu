#pragma once

#include "Looper.h"
#include <functional>
#include <atomic>

namespace setu {
namespace os {

class Handler {
public:
    Handler(Looper* looper = nullptr);
    virtual ~Handler();

    // Callbacks API
    uint64_t post(std::function<void()> r);
    uint64_t postDelayed(std::function<void()> r, long long delayMillis);
    
    // In Setu, to map Dalvik objects exactly, we can also pass a Dalvik obj token
    void postDelayedWithDalvikObj(std::function<void()> r, long long delayMillis, void* dalvikObj);
    
    void removeCallbacks(void* dalvikObj);
    void removeCallbacksByToken(uint64_t token);

    // Messages API
    bool sendMessage(Message* msg);
    bool sendMessageDelayed(Message* msg, long long delayMillis);
    bool sendMessageAtTime(Message* msg, long long uptimeMillis);

    void removeCallbacksAndMessages(void* obj);

    virtual void handleMessage(Message* msg);
    void dispatchMessage(Message* msg);
    
    Looper* getLooper() const { return m_looper; }

private:
    Looper* m_looper;
    MessageQueue* m_queue;
    static std::atomic<uint64_t> s_tokenCounter;
    
    Message* getPostMessage(std::function<void()> r, void* dalvikObj, uint64_t token);
};

} // namespace os
} // namespace setu
