#pragma once

#include <cstddef>
#include <optional>

namespace incfs {

class IncFsFileMap {
public:
    IncFsFileMap() {}
    ~IncFsFileMap() {}
    bool Create(int fd, off64_t offset, size_t length, const char* file_name, bool verify = false) {
        return false; // Stub
    }
};

template <typename T>
class map_ptr {
    const T* ptr_;
public:
    using const_iterator = map_ptr<T>;

    using iterator_category = std::random_access_iterator_tag;
    using value_type = map_ptr<T>;
    using difference_type = ptrdiff_t;
    using pointer = map_ptr<T>*;
    using reference = map_ptr<T>;

    map_ptr(const T* p = nullptr) : ptr_(p) {}
    
    map_ptr<T> iterator() const { return *this; }
    
    const T* get() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    const T* operator->() const { return ptr_; }
    map_ptr<T> operator*() const { return map_ptr<T>(ptr_); }
    
    map_ptr<T> operator+(ptrdiff_t offset) const {
        return map_ptr<T>(ptr_ + offset);
    }
    map_ptr<T> operator-(ptrdiff_t offset) const {
        return map_ptr<T>(ptr_ - offset);
    }
    
    map_ptr<T>& operator+=(ptrdiff_t offset) {
        ptr_ += offset;
        return *this;
    }
    map_ptr<T>& operator-=(ptrdiff_t offset) {
        ptr_ -= offset;
        return *this;
    }
    map_ptr<T>& operator++() {
        ++ptr_;
        return *this;
    }
    map_ptr<T> operator++(int) {
        map_ptr<T> temp(ptr_);
        ++ptr_;
        return temp;
    }
    map_ptr<T>& operator--() {
        --ptr_;
        return *this;
    }
    map_ptr<T> operator--(int) {
        map_ptr<T> temp(ptr_);
        --ptr_;
        return temp;
    }
    
    ptrdiff_t operator-(const map_ptr<T>& other) const {
        return ptr_ - other.ptr_;
    }

    
    bool operator==(const map_ptr<T>& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const map_ptr<T>& o) const { return ptr_ != o.ptr_; }
    
    bool operator==(std::nullptr_t) const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr_ != nullptr; }
    
    bool operator<(const map_ptr<T>& o) const { return ptr_ < o.ptr_; }
    bool operator>(const map_ptr<T>& o) const { return ptr_ > o.ptr_; }
    bool operator<=(const map_ptr<T>& o) const { return ptr_ <= o.ptr_; }
    bool operator>=(const map_ptr<T>& o) const { return ptr_ >= o.ptr_; }
    
    // cast operator
    template <typename U>
    map_ptr<U> convert() const { return map_ptr<U>(reinterpret_cast<const U*>(ptr_)); }
    
    map_ptr<T> offset(size_t n) const {
        return map_ptr<T>(reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(ptr_) + n));
    }
    
    const T* verified() const { return ptr_; }
    const T* verify() const { return ptr_; }
    map_ptr<T> verify(size_t length) const { return *this; }
    const T* unsafe_ptr() const { return ptr_; }
    T value() const { return *ptr_; }
    
    operator map_ptr<void>() const { return map_ptr<void>(ptr_); }
};

template <>
class map_ptr<void> {
    const void* ptr_;
public:
    map_ptr(const void* p = nullptr) : ptr_(p) {}
    
    const void* get() const { return ptr_; }
    explicit operator bool() const { return ptr_ != nullptr; }
    
    bool operator==(const map_ptr<void>& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const map_ptr<void>& o) const { return ptr_ != o.ptr_; }
    
    bool operator==(std::nullptr_t) const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t) const { return ptr_ != nullptr; }
    
    template <typename U>
    map_ptr<U> convert() const { return map_ptr<U>(reinterpret_cast<const U*>(ptr_)); }
    
    map_ptr<void> offset(size_t n) const {
        return map_ptr<void>(reinterpret_cast<const uint8_t*>(ptr_) + n);
    }
    
    const void* verified() const { return ptr_; }
    const void* verify() const { return ptr_; }
    map_ptr<void> verify(size_t length) const { return *this; }
    const void* unsafe_ptr() const { return ptr_; }
};

template <typename T>
using verified_map_ptr = map_ptr<T>;

template <typename T>
map_ptr<T> make_map_ptr(const T* p) {
    return map_ptr<T>(p);
}

} // namespace incfs

namespace android {
namespace util {
    using incfs::map_ptr;
    using incfs::make_map_ptr;
}
}
