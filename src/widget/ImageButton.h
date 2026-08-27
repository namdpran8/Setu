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

#include <cstdint>
#include <string>

#include "ImageView.h"

namespace setu {
namespace widget {

// android.widget.ImageButton: an ImageView that behaves like a button.
//
// ImageButton.java is almost empty - it calls setFocusable(true) and blocks
// onSetAlpha, and everything else that makes an ImageButton look and act like one
// comes from Widget.ImageButton in themes.xml:
//
//     <item name="focusable">true</item>
//     <item name="clickable">true</item>
//     <item name="scaleType">center</item>
//     <item name="background">@drawable/btn_default</item>
//
// LayoutInflater already passes imageButtonStyle as the defStyleAttr, so
// ImageView's own attribute read picks up scaleType from that style whenever the
// framework resolves. This class supplies the two things that read cannot: the
// background (android:background is not in the ImageView styleable - it belongs to
// View, which reads nothing here), and the touch handling that turns a press into
// a state change the background can respond to.
//
// The style defaults are also applied as fallbacks, for the case where the theme
// does not resolve. An ImageButton that came out non-clickable and fitCenter-scaled
// because framework-res was unavailable would be a confusing way to fail.
class ImageButton : public ImageView {
public:
    ImageButton(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser,
                uint32_t defStyleAttr, uint32_t defStyleRes);
    ImageButton();
    ~ImageButton() override = default;

    // Reports the press through setPressed(), so the background <selector> decides
    // what a pressed ImageButton looks like - the same contract as Button. Needed
    // because View::onTouchEvent fires the click without ever entering the pressed
    // state, which leaves a stock btn_default looking inert under the finger.
    bool onTouchEvent(view::MotionEvent& event) override;

    std::string getClassName() const override { return "ImageButton"; }
};

} // namespace widget
} // namespace setu
