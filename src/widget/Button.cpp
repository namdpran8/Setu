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

#include "Button.h"
#include "../view/MotionEvent.h"
#include "../utils/Logger.h"

namespace setu {
namespace widget {

Button::Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes)
    : TextView(resManager, theme, parser, defStyleAttr, defStyleRes) {
    // View and TextView constructors handle parsing all styling attributes (background, minWidth, minHeight, padding, gravity, text styling, etc).
    // The AOSP Button style (Widget.Button or Widget.Material.Button) fully defines the appearance and padding (via background padding box).
}

Button::Button() : TextView() {
}

bool Button::onTouchEvent(view::MotionEvent& event) {
    if (!isEnabled()) return true;

    if (event.getAction() == view::MotionEvent::Action::DOWN) {
        Logger::d("Button", "Action::DOWN received");
        drawableHotspotChanged(event.getX(), event.getY());
        setPressed(true);
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::MOVE) {
        drawableHotspotChanged(event.getX(), event.getY());
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::UP) {
        Logger::d("Button", "Action::UP received, performing click");
        setPressed(false);
        performClick();
        return true;
    } else if (event.getAction() == view::MotionEvent::Action::CANCEL) {
        Logger::d("Button", "Action::CANCEL received");
        setPressed(false);
        return true;
    }
    return false;
}

} // namespace widget
} // namespace setu
