#pragma once
#include "../view/View.h"
#include "../graphics/Paint.h"
#include <string>

namespace windroid {
namespace widget {

class TextView : public view::View {
public:
    TextView(ResourceManager* resManager, Theme* theme, const struct AxmlNode* node, uint32_t defStyleAttr, uint32_t defStyleRes);
    TextView();
    virtual ~TextView() = default;

    void setText(const std::wstring& text);
    const std::wstring& getText() const { return mText; }

    void setTextColor(uint32_t color);
    void setTextSize(float size);

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onDraw(graphics::Canvas& canvas) override;

private:
    std::wstring mText;
    graphics::Paint mTextPaint;
};

} // namespace widget
} // namespace windroid
