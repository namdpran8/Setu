#include "ColorDrawable.h"

#include "../Canvas.h"

namespace setu {
namespace graphics {

void ColorDrawable::setColor(uint32_t color) {
    if (mColor == color) return;
    mColor = color;
    invalidateSelf();
}

void ColorDrawable::setAlpha(int alpha) {
    alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
    if (mAlpha == alpha) return;
    mAlpha = alpha;
    invalidateSelf();
}

uint32_t ColorDrawable::resolveColor() const {
    if (mAlpha >= 255) return mColor;
    const uint32_t baseAlpha = (mColor >> 24) & 0xFF;
    const uint32_t scaled = (baseAlpha * (uint32_t)mAlpha) / 255u;
    return (scaled << 24) | (mColor & 0x00FFFFFF);
}

void ColorDrawable::draw(Canvas& canvas) {
    const uint32_t color = resolveColor();
    // Fully transparent is the default for a View with no background; skip the
    // draw entirely rather than pushing a no-op fill through the display list.
    if ((color >> 24) == 0) return;

    const Rect& b = getBounds();
    if (b.isEmpty()) return;

    Paint paint;
    paint.setColor(color);
    paint.setStyle(Style::FILL);
    canvas.drawRect((float)b.left, (float)b.top, (float)b.right, (float)b.bottom, paint);
}

} // namespace graphics
} // namespace setu
