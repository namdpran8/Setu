/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
    paint.setColorFilter(getActiveColorFilter());
    canvas.drawRect((float)b.left, (float)b.top, (float)b.right, (float)b.bottom, paint);
}

} // namespace graphics
} // namespace setu
