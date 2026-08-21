#pragma once
#include "../view/View.h"
#include "../graphics/Paint.h"
#include <string>

namespace setu {
namespace widget {

class TextView : public view::View {
public:
    TextView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    TextView();
    virtual ~TextView() = default;

    void setText(const std::wstring& text);
    const std::wstring& getText() const { return mText; }

    void setTextColor(uint32_t color);
    void setTextSize(float size);

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onDraw(graphics::Canvas& canvas) override;

protected:
    std::wstring mText;
    graphics::Paint mTextPaint;
    int mGravity = 0x33; // Gravity::TOP | Gravity::LEFT
    int mEms = -1;
};

} // namespace widget
} // namespace setu
