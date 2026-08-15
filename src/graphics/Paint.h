#pragma once

#include <cstdint>

namespace setu {
namespace graphics {

enum class Style {
    FILL,
    STROKE,
    FILL_AND_STROKE
};

class Paint {
public:
    Paint();
    ~Paint() = default;

    void setColor(uint32_t color); // ARGB
    uint32_t getColor() const;

    void setStyle(Style style);
    Style getStyle() const;

    void setStrokeWidth(float width);
    float getStrokeWidth() const;

    void setTextSize(float textSize);
    float getTextSize() const;

    void setAntiAlias(bool aa);
    bool isAntiAlias() const;

private:
    uint32_t mColor; // ARGB
    Style mStyle;
    float mStrokeWidth;
    float mTextSize;
    bool mAntiAlias;
};

} // namespace graphics
} // namespace setu
