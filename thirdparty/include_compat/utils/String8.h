#pragma once

#include <string>
#include <stdarg.h>
#include <android-base/stringprintf.h>

namespace android {

class String16;

class String8 : public std::string {
public:
    String8() : std::string() {}
    String8(const char* s) : std::string(s ? s : "") {}
    String8(const char* s, size_t len) : std::string(s, len) {}
    String8(const String8& o) : std::string(o) {}
    
    // forward declaration workaround by using template
    template<typename T>
    String8(const T& o, typename std::enable_if<std::is_same<T, class String16>::value>::type* = 0) {
        const char16_t* s = o.string();
        if (s) while (*s) push_back((char)*s++);
    }
    String8(const char16_t* s) {
        if (s) while (*s) push_back((char)*s++);
    }
    String8(const char16_t* s, size_t len) {
        if (s) for (size_t i = 0; i < len; i++) push_back((char)s[i]);
    }
    
    void appendFormat(const char* fmt, ...) {
        va_list ap;
        va_start(ap, fmt);
        android::base::StringAppendV(this, fmt, ap);
        va_end(ap);
    }
    
    const char* string() const { return c_str(); }
    void setTo(const char* s) { assign(s); }
    void setTo(const char* s, size_t len) { assign(s, len); }
    size_t length() const { return size(); }
};

} // namespace android
