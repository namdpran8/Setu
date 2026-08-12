#pragma once
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

namespace android {
    class Mutex {
        std::mutex mMutex;
    public:
        void lock() { mMutex.lock(); }
        void unlock() { mMutex.unlock(); }
        bool tryLock() { return mMutex.try_lock(); }
        std::mutex& get() { return mMutex; }
    };

    class AutoMutex {
        Mutex& mLock;
    public:
        AutoMutex(Mutex& mutex) : mLock(mutex) { mLock.lock(); }
        ~AutoMutex() { mLock.unlock(); }
    };

    class RWLock {
        std::shared_mutex mLock;
    public:
        void readLock() { mLock.lock_shared(); }
        void unlock() { mLock.unlock(); } // standard unlock is often just unlock
        void writeLock() { mLock.lock(); }
    };

    class AutoRLock {
        RWLock& mLock;
    public:
        AutoRLock(RWLock& lock) : mLock(lock) { mLock.readLock(); }
        ~AutoRLock() { mLock.unlock(); }
    };

    class AutoWLock {
        RWLock& mLock;
    public:
        AutoWLock(RWLock& lock) : mLock(lock) { mLock.writeLock(); }
        ~AutoWLock() { mLock.unlock(); }
    };
}
