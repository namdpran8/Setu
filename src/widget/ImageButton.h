#pragma once
#include "ImageView.h"

namespace setu {
namespace widget {

class ImageButton : public ImageView {
public:
    ImageButton(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes) 
        : ImageView(resManager, theme, parser, defStyleAttr, defStyleRes) {}
    ImageButton() : ImageView() {}
    virtual ~ImageButton() = default;

    virtual std::string getClassName() const override { return "ImageButton"; }
};

} // namespace widget
} // namespace setu
