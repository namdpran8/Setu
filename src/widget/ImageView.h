#pragma once
#include "../view/View.h"

namespace setu {
namespace widget {

class ImageView : public view::View {
public:
    ImageView(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes) 
        : View(resManager, theme, parser, defStyleAttr, defStyleRes) {}
    ImageView();
    virtual ~ImageView() = default;

    virtual void onDraw(graphics::Canvas& canvas) override;
    virtual void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;

    virtual std::string getClassName() const override { return "ImageView"; }
};

} // namespace widget
} // namespace setu
