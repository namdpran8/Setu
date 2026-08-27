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

#pragma once

#include "Drawable.h"
#include "../Paint.h"

namespace setu {
namespace graphics {

// android.graphics.drawable.ColorDrawable: fills its bounds with one ARGB value.
//
// This is what View::setBackgroundColor() now produces, so the "background is a
// plain colour" case and the "background is a <shape>" case travel through
// exactly the same code path in View::draw().
class ColorDrawable : public Drawable {
public:
    ColorDrawable() = default;
    explicit ColorDrawable(uint32_t color) : mColor(color) {}

    void draw(Canvas& canvas) override;

    // The colour as authored, ignoring any setAlpha() applied on top.
    uint32_t getColor() const { return mColor; }
    void setColor(uint32_t color);

    void setAlpha(int alpha) override;
    int getAlpha() const override { return mAlpha; }

private:
    // The colour actually painted: authored colour with mAlpha folded into its
    // alpha channel, the way AOSP's ColorDrawable does in updateLocalState().
    uint32_t resolveColor() const;

    uint32_t mColor = 0x00000000;
    int mAlpha = 255;
};

} // namespace graphics
} // namespace setu
