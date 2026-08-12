#pragma once

#include <stdint.h>
#include <string.h>
#include <windows.h>

#undef ERROR
#undef NO_ERROR

#ifdef __cplusplus
extern "C" {
#endif

inline ssize_t utf8_to_utf16_length(const uint8_t* src, size_t srcLen) {
    if (srcLen == 0) return 0;
    return MultiByteToWideChar(CP_UTF8, 0, (LPCCH)src, (int)srcLen, NULL, 0);
}

inline char16_t* utf8_to_utf16(const uint8_t* src, size_t srcLen, char16_t* dst, size_t dstLen) {
    if (srcLen == 0 || dstLen == 0) {
        if (dstLen > 0) dst[0] = 0;
        return dst;
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)src, (int)srcLen, (LPWSTR)dst, (int)dstLen - 1);
    if (len >= 0 && len < (int)dstLen) {
        dst[len] = 0;
    }
    return dst + len;
}

inline ssize_t utf16_to_utf8_length(const char16_t* src, size_t srcLen) {
    if (srcLen == 0) return 0;
    return WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)src, (int)srcLen, NULL, 0, NULL, NULL);
}

inline void utf16_to_utf8(const char16_t* src, size_t srcLen, char* dst, size_t dstLen) {
    if (srcLen == 0 || dstLen == 0) {
        if (dstLen > 0) dst[0] = '\0';
        return;
    }
    int len = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH)src, (int)srcLen, dst, (int)dstLen - 1, NULL, NULL);
    if (len >= 0 && len < (int)dstLen) {
        dst[len] = '\0';
    }
}

inline int strncmp16(const char16_t* s1, const char16_t* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return s1[i] < s2[i] ? -1 : 1;
        if (s1[i] == 0) return 0;
    }
    return 0;
}

inline ssize_t utf32_to_utf8_length(const char32_t* src, size_t src_len) {
    size_t len = 0;
    for (size_t i = 0; i < src_len; i++) {
        if (src[i] < 0x80) len += 1;
        else if (src[i] < 0x800) len += 2;
        else if (src[i] < 0x10000) len += 3;
        else len += 4;
    }
    return (ssize_t)len;
}

inline int strzcmp16(const char16_t *s1, size_t n1, const char16_t *s2, size_t n2) {
    const char16_t* e1 = s1 + n1;
    const char16_t* e2 = s2 + n2;

    while (s1 < e1 && s2 < e2 && *s1 != 0 && *s2 != 0 && *s1 == *s2) {
        s1++;
        s2++;
    }

    if ((s1 == e1 || *s1 == 0) && (s2 == e2 || *s2 == 0)) {
        return 0;
    }
    if (s1 == e1 || *s1 == 0) {
        return -1;
    }
    if (s2 == e2 || *s2 == 0) {
        return 1;
    }
    return (int)*s1 - (int)*s2;
}

#ifdef __cplusplus
} // extern "C"

inline ssize_t utf8_to_utf16_length(const uint8_t* src, size_t srcLen, bool out_error) {
    return utf8_to_utf16_length(src, srcLen);
}
#endif



inline void utf32_to_utf8(const char32_t* src, size_t src_len, char* dst, size_t dst_len) {
    if (dst_len > 0) dst[0] = '\0';
}

inline int32_t utf32_from_utf8_at(const char *src, size_t src_len, size_t index, size_t *next_index) {
    if (next_index) *next_index = index + 1;
    if (index < src_len) return src[index];
    return 0;
}

