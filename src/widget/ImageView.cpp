#include "ImageView.h"

namespace setu {
namespace widget {

ImageView::ImageView() : view::View(nullptr, nullptr, nullptr, 0, 0) {}

void ImageView::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // For now, simple fallback, measure as 100x100 if unspecified
    int width = view::View::getSize(widthMeasureSpec);
    int height = view::View::getSize(heightMeasureSpec);
    
    if (view::View::getMode(widthMeasureSpec) == view::View::MEASURE_SPEC_UNSPECIFIED) {
        width = 100;
    }
    if (view::View::getMode(heightMeasureSpec) == view::View::MEASURE_SPEC_UNSPECIFIED) {
        height = 100;
    }

    setMeasuredDimension(width, height);
}

void ImageView::onDraw(graphics::Canvas& canvas) {
    // Draw background/fallback for image
    view::View::onDraw(canvas);
}

} // namespace widget
} // namespace setu
