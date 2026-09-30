#pragma once

#include <string>
#include <memory>

namespace setu {
namespace graphics {

class Typeface {
public:
    enum Style {
        NORMAL = 0,
        BOLD = 1,
        ITALIC = 2,
        BOLD_ITALIC = 3
    };

    Typeface(const std::string& familyName, Style style)
        : mFamilyName(familyName), mStyle(style) {}

    static std::shared_ptr<Typeface> create(const std::string& familyName, Style style) {
        return std::make_shared<Typeface>(familyName, style);
    }

    const std::string& getFamilyName() const { return mFamilyName; }
    Style getStyle() const { return mStyle; }

private:
    std::string mFamilyName;
    Style mStyle;
};

using TypefacePtr = std::shared_ptr<Typeface>;

} // namespace graphics
} // namespace setu
