#pragma once

// FileMap.h — Windows implementation of Android's FileMap using
// CreateFileMapping / MapViewOfFile.

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <windows.h>
#include <io.h>     // _get_osfhandle
#include <cstddef>
#include <cstdint>
#include <string>

// Clean up macros that conflict with Android headers.
#ifdef ERROR
#  undef ERROR
#endif
#ifdef NO_ERROR
#  undef NO_ERROR
#endif

namespace android {

class FileMap {
public:
    FileMap() = default;

    ~FileMap() {
        if (view_)    ::UnmapViewOfFile(view_);
        if (mapping_) ::CloseHandle(mapping_);
    }

    FileMap(const FileMap&) = delete;
    FileMap& operator=(const FileMap&) = delete;

    // Matches the Android FileMap::create() signature used by ZipFileRO.
    bool create(const char* origFileName, int fd,
                off64_t offset, size_t length, bool /*readOnly*/) {
        HANDLE hFile = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));
        if (hFile == INVALID_HANDLE_VALUE) return false;

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

        data_     = static_cast<uint8_t*>(view_) + adjust;
        length_   = length;
        file_name_ = origFileName ? origFileName : "";
        return true;
    }

    void*  getDataPtr()  const { return data_; }
    size_t getDataLength() const { return length_; }
    const char* getFileName() const { return file_name_.c_str(); }

private:
    void*       mapping_   = nullptr;
    void*       view_      = nullptr;
    uint8_t*    data_      = nullptr;
    size_t      length_    = 0;
    std::string file_name_;
};

} // namespace android
