#pragma once

#include "Message.h"
#include <mutex>

namespace setu {
namespace os {

class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue();

    bool enqueueMessage(Message* msg, long long when);
    Message* next();
    
    // Remove all messages with target and obj
    void removeCallbacksAndMessages(Handler* h, void* obj);
    
    // Remove specific callback by Dalvik object
    void removeCallbacks(Handler* h, void* runnableObj);
    
    // Remove specific callback by C++ token
    void removeCallbacksByToken(Handler* h, uint64_t token);

    void quit();
    
private:
    std::mutex m_mutex;
    Message* m_messages;
    bool m_quitting;
    
    void wake(long long delay);
};

} // namespace os
} // namespace setu
