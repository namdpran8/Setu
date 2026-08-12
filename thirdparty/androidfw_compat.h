#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifdef _MSC_VER

#include <io.h>
#include <limits>

template <typename T1, typename T2, typename T3>
inline bool __builtin_mul_overflow(T1 a, T2 b, T3* res) {
    T3 a3 = static_cast<T3>(a);
    T3 b3 = static_cast<T3>(b);
    if (a3 != 0 && b3 > (std::numeric_limits<T3>::max)() / a3) return true;
    *res = a3 * b3;
    return false;
}

template <typename T1, typename T2, typename T3>
inline bool __builtin_add_overflow(T1 a, T2 b, T3* res) {
    T3 a3 = static_cast<T3>(a);
    T3 b3 = static_cast<T3>(b);
    if (b3 > (std::numeric_limits<T3>::max)() - a3) return true;
    *res = a3 + b3;
    return false;
}
#define lseek64 _lseeki64
#define S_ISBLK(m) 0
// TEMP_FAILURE_RETRY: on Linux retries a syscall on EINTR; no EINTR on Windows.
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) (exp)
#endif
#include <process.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef OS_PATH_SEPARATOR
#define OS_PATH_SEPARATOR '\\'
#endif

// Log stubs
#include <stdio.h>
#define ALOGE(...) do { fprintf(stderr, "E: " __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define ALOGW(...) do { fprintf(stderr, "W: " __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define ALOGI(...) do { fprintf(stdout, "I: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define ALOGD(...) do { fprintf(stdout, "D: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define ALOGV(...) do { fprintf(stdout, "V: " __VA_ARGS__); fprintf(stdout, "\n"); } while(0)

#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#ifndef S_IRGRP
#define S_IRGRP 0
#endif
#ifndef S_IROTH
#define S_IROTH 0
#endif

#ifndef SSIZE_MAX
#ifdef _WIN64
#define SSIZE_MAX _I64_MAX
#else
#define SSIZE_MAX INT_MAX
#endif
#endif

// Disable non-standard warnings
#pragma warning(disable : 4068) // unknown pragma
#pragma warning(disable : 4200) // zero-sized array in struct/union

// Missing POSIX types and functions
typedef int mode_t;
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
typedef __int64 off64_t;

#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define PATH_MAX 260
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef F_OK
#define F_OK 0
#endif

// Windows undefs
#undef NO_ERROR // Conflicts with AOSP NO_ERROR enum/defines
#undef ERROR    // Conflicts with AOSP ERROR log severity

// Missing libc functions
#define fseeko _fseeki64
#define ftello _ftelli64
#define ftruncate _chsize_s
#define localtime_r(timep, result) (localtime_s((result), (timep)) == 0 ? (result) : nullptr)

#ifndef powerof2
#define powerof2(x) ((((x)-1)&(x))==0)
#endif

#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif
#endif

// GCC __attribute__ wrappers
#ifndef __attribute__
#define __attribute__(x)
#endif

#define __PRETTY_FUNCTION__ __FUNCSIG__

#ifndef PACKED_STRUCT
#define PACKED_STRUCT __pragma(pack(push, 1)) struct
#endif

#ifndef PACKED_STRUCT_END
#define PACKED_STRUCT_END __pragma(pack(pop))
#endif

// Some headers define this if not defined
#ifndef L_tmpnam
#define L_tmpnam 260
#endif

#endif // _MSC_VER
