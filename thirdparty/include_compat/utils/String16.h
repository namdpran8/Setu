#pragma once

#include <string>

namespace android {

class String16 : public std::u16string {
public:
    String16() : std::u16string() {}
    String16(const char16_t* s) : std::u16string(s ? s : u"") {}
    String16(const char16_t* s, size_t len) : std::u16string(s, len) {}
    String16(const String16& o) : std::u16string(o) {}
    String16(const char* s) {
        if (s) while (*s) push_back((char16_t)*s++);
    }
    String16(const char* s, size_t len) {
        if (s) for (size_t i = 0; i < len; i++) push_back((char16_t)s[i]);
    }
    
    const char16_t* string() const { return c_str(); }
    void setTo(const char16_t* s) { assign(s); }
    void setTo(const char16_t* s, size_t len) { assign(s, len); }
    size_t length() const { return size(); }
};

} // namespace android
