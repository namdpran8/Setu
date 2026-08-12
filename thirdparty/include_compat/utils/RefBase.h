#pragma once
#include <atomic>

namespace android {

class RefBase {
public:
    void incStrong(const void* id) const { mCount++; }
    void decStrong(const void* id) const { if (--mCount == 0) delete this; }
protected:
    RefBase() : mCount(0) {}
    virtual ~RefBase() {}
private:
    mutable std::atomic<int> mCount;
};

template <typename T> class sp {
    T* mPtr;
public:
    sp(T* p = nullptr) : mPtr(p) { if (mPtr) mPtr->incStrong(this); }
    sp(const sp<T>& o) : mPtr(o.mPtr) { if (mPtr) mPtr->incStrong(this); }
    ~sp() { if (mPtr) mPtr->decStrong(this); }
    
    T* get() const { return mPtr; }
    void clear() { if (mPtr) { mPtr->decStrong(this); mPtr = nullptr; } }
    
    template <typename... Args>
    static sp<T> make(Args&&... args) {
        return sp<T>(new T(std::forward<Args>(args)...));
    }
    
    T* operator->() const { return mPtr; }
    T& operator*() const { return *mPtr; }
    
    bool operator<(const sp<T>& o) const { return mPtr < o.mPtr; }
    bool operator==(const sp<T>& o) const { return mPtr == o.mPtr; }
    bool operator!=(const sp<T>& o) const { return mPtr != o.mPtr; }
    bool operator==(T* o) const { return mPtr == o; }
    bool operator!=(T* o) const { return mPtr != o; }
    
    explicit operator bool() const { return mPtr != nullptr; }
    

    sp<T>& operator=(const sp<T>& o) {
        if (o.mPtr) o.mPtr->incStrong(this);
        if (mPtr) mPtr->decStrong(this);
        mPtr = o.mPtr;
        return *this;
    }
};

template <typename T> class wp {
    T* mPtr;
public:
    wp(T* p = nullptr) : mPtr(p) {}
    wp(const sp<T>& o) : mPtr(o.get()) {}
    wp(const wp<T>& o) : mPtr(o.mPtr) {}
    ~wp() {}
    sp<T> promote() const { return sp<T>(mPtr); }
    bool operator==(const wp<T>& o) const { return mPtr == o.mPtr; }
    wp<T>& operator=(const sp<T>& o) { mPtr = o.get(); return *this; }
    wp<T>& operator=(const wp<T>& o) { mPtr = o.mPtr; return *this; }
};

} // namespace android
