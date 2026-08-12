#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// Forward-declare map_ptr before IncFsFileMap (which returns map_ptr<void>).
namespace incfs {
template <typename T> class map_ptr;
template <>           class map_ptr<void>; // explicit specialisation forward-decl
} // namespace incfs

// Pull in Windows headers with minimal pollution.
// Must come before namespace incfs so that macros like ERROR can be cleaned up
// before Android headers (Errors.h) are included.
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  include <io.h>   // _get_osfhandle
// windows.h defines ERROR=0 which breaks android::utils::Errors enum.
#  ifdef ERROR
#    undef ERROR
#  endif
#  ifdef NO_ERROR
#    undef NO_ERROR
#  endif
#endif // _WIN32

namespace incfs {

// ---------------------------------------------------------------------------
// IncFsFileMap – Windows memory-mapped file wrapper
// ---------------------------------------------------------------------------
class IncFsFileMap {
public:
    IncFsFileMap() = default;

    ~IncFsFileMap() {
#ifdef _WIN32
        if (view_)    ::UnmapViewOfFile(view_);
        if (mapping_) ::CloseHandle(mapping_);
#endif
    }

    // Non-copyable
    IncFsFileMap(const IncFsFileMap&) = delete;
    IncFsFileMap& operator=(const IncFsFileMap&) = delete;

    // Movable
    IncFsFileMap(IncFsFileMap&& o) noexcept
        : mapping_(o.mapping_), view_(o.view_),
          data_ptr_(o.data_ptr_), length_(o.length_),
          offset_(o.offset_), file_name_(std::move(o.file_name_)) {
        o.mapping_ = nullptr; o.view_ = nullptr; o.data_ptr_ = nullptr;
    }
    IncFsFileMap& operator=(IncFsFileMap&& o) noexcept {
        if (this != &o) {
#ifdef _WIN32
            if (view_)    ::UnmapViewOfFile(view_);
            if (mapping_) ::CloseHandle(mapping_);
#endif
            mapping_   = o.mapping_;   o.mapping_  = nullptr;
            view_      = o.view_;      o.view_     = nullptr;
            data_ptr_  = o.data_ptr_;  o.data_ptr_ = nullptr;
            length_    = o.length_;
            offset_    = o.offset_;
            file_name_ = std::move(o.file_name_);
        }
        return *this;
    }

    bool Create(int fd, off64_t offset, size_t length,
                const char* file_name, bool /*verify*/ = false) {
#ifdef _WIN32
        HANDLE hFile = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
        if (hFile == INVALID_HANDLE_VALUE) return false;

        // MapViewOfFile requires the offset to be aligned to the system
        // allocation granularity (typically 64 KB on Windows).
        SYSTEM_INFO si;
        ::GetSystemInfo(&si);
        const DWORD gran = si.dwAllocationGranularity;
        const off64_t aligned = (offset / gran) * gran;
        const size_t  adjust  = static_cast<size_t>(offset - aligned);

        mapping_ = ::CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping_) return false;

        const DWORD hi = static_cast<DWORD>(static_cast<uint64_t>(aligned) >> 32);
        const DWORD lo = static_cast<DWORD>(static_cast<uint64_t>(aligned) & 0xFFFFFFFFu);
        view_ = ::MapViewOfFile(mapping_, FILE_MAP_READ, hi, lo, length + adjust);
        if (!view_) {
            ::CloseHandle(mapping_); mapping_ = nullptr;
            return false;
        }

        data_ptr_  = static_cast<const uint8_t*>(view_) + adjust;
        length_    = length;
        offset_    = offset;
        file_name_ = file_name ? file_name : "";
        return true;
#else
        return false;
#endif
    }

    map_ptr<void> data() const; // defined after map_ptr<void> specialisation
    const void*   unsafe_data() const { return data_ptr_; }
    size_t        length()    const { return length_; }
    off64_t       offset()    const { return offset_; }
    const char*   file_name() const { return file_name_.c_str(); }

private:
    void*          mapping_  = nullptr; // HANDLE on Windows
    void*          view_     = nullptr; // mapped view base
    const uint8_t* data_ptr_ = nullptr; // adjusted pointer into the view
    size_t         length_   = 0;
    off64_t        offset_   = 0;
    std::string    file_name_;
};

// ---------------------------------------------------------------------------
// map_ptr<T> – thin wrapper around a const pointer (no-op on non-IncFs builds)
// ---------------------------------------------------------------------------
template <typename T>
class map_ptr {
    const T* ptr_;
public:
    using const_iterator     = map_ptr<T>;
    using iterator_category  = std::random_access_iterator_tag;
    using value_type         = map_ptr<T>;
    using difference_type    = ptrdiff_t;
    using pointer            = map_ptr<T>*;
    using reference          = map_ptr<T>;

    map_ptr(const T* p = nullptr) : ptr_(p) {}

    map_ptr<T> iterator() const { return *this; }

    const T* get()              const { return ptr_; }
    explicit operator bool()    const { return ptr_ != nullptr; }

    const T*   operator->()     const { return ptr_; }
    map_ptr<T> operator*()      const { return map_ptr<T>(ptr_); }

    map_ptr<T> operator+(ptrdiff_t n) const { return map_ptr<T>(ptr_ + n); }
    map_ptr<T> operator-(ptrdiff_t n) const { return map_ptr<T>(ptr_ - n); }

    map_ptr<T>& operator+=(ptrdiff_t n) { ptr_ += n; return *this; }
    map_ptr<T>& operator-=(ptrdiff_t n) { ptr_ -= n; return *this; }
    map_ptr<T>& operator++()            { ++ptr_; return *this; }
    map_ptr<T>  operator++(int)         { map_ptr<T> t(ptr_); ++ptr_; return t; }
    map_ptr<T>& operator--()            { --ptr_; return *this; }
    map_ptr<T>  operator--(int)         { map_ptr<T> t(ptr_); --ptr_; return t; }

    ptrdiff_t operator-(const map_ptr<T>& o) const { return ptr_ - o.ptr_; }

    bool operator==(const map_ptr<T>& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const map_ptr<T>& o) const { return ptr_ != o.ptr_; }
    bool operator==(std::nullptr_t)      const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t)      const { return ptr_ != nullptr; }
    bool operator< (const map_ptr<T>& o) const { return ptr_ <  o.ptr_; }
    bool operator> (const map_ptr<T>& o) const { return ptr_ >  o.ptr_; }
    bool operator<=(const map_ptr<T>& o) const { return ptr_ <= o.ptr_; }
    bool operator>=(const map_ptr<T>& o) const { return ptr_ >= o.ptr_; }

    template <typename U>
    map_ptr<U> convert() const {
        return map_ptr<U>(reinterpret_cast<const U*>(ptr_));
    }

    map_ptr<T> offset(size_t n) const {
        return map_ptr<T>(reinterpret_cast<const T*>(
            reinterpret_cast<const uint8_t*>(ptr_) + n));
    }

    const T* verified()              const { return ptr_; }
    const T* verify()                const { return ptr_; }
    map_ptr<T> verify(size_t)        const { return *this; }
    const T* unsafe_ptr()            const { return ptr_; }
    T        value()                 const { return *ptr_; }

    operator map_ptr<void>() const; // defined after map_ptr<void> specialisation
};

// ---------------------------------------------------------------------------
// map_ptr<void> specialisation
// ---------------------------------------------------------------------------
template <>
class map_ptr<void> {
    const void* ptr_;
public:
    map_ptr(const void* p = nullptr) : ptr_(p) {}

    const void* get()           const { return ptr_; }
    explicit operator bool()    const { return ptr_ != nullptr; }

    bool operator==(const map_ptr<void>& o) const { return ptr_ == o.ptr_; }
    bool operator!=(const map_ptr<void>& o) const { return ptr_ != o.ptr_; }
    bool operator==(std::nullptr_t)         const { return ptr_ == nullptr; }
    bool operator!=(std::nullptr_t)         const { return ptr_ != nullptr; }

    template <typename U>
    map_ptr<U> convert() const {
        return map_ptr<U>(reinterpret_cast<const U*>(ptr_));
    }

    map_ptr<void> offset(size_t n) const {
        return map_ptr<void>(reinterpret_cast<const uint8_t*>(ptr_) + n);
    }

    const void* verified()          const { return ptr_; }
    const void* verify()            const { return ptr_; }
    map_ptr<void> verify(size_t)    const { return *this; }
    const void* unsafe_ptr()        const { return ptr_; }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
// Out-of-line definitions — map_ptr<void> is now fully defined.
// ---------------------------------------------------------------------------
template <typename T>
inline map_ptr<T>::operator map_ptr<void>() const {
    return map_ptr<void>(ptr_);
}

inline map_ptr<void> IncFsFileMap::data() const {
    return map_ptr<void>(data_ptr_);
}

template <typename T>
using verified_map_ptr = map_ptr<T>;

template <typename T>
map_ptr<T> make_map_ptr(const T* p) { return map_ptr<T>(p); }

} // namespace incfs

namespace android {
namespace util {
    using incfs::map_ptr;
    using incfs::make_map_ptr;
}
}
