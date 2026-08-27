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
#include "TextView.h"
#include <functional>
#include <memory>

namespace setu {
namespace widget {

class Button : public TextView {
public:
    Button(ResourceManager* resManager, Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);
    Button();
    virtual ~Button() = default;

    // Reports the press through setPressed(), so the background <selector> is what
    // decides how a pressed button looks. Nothing here knows about colours any
    // more, which is why an app's own selector now works as well as the built-in
    // one.
    bool onTouchEvent(view::MotionEvent& event) override;

    std::string getClassName() const override { return "Button"; }
};

} // namespace widget
} // namespace setu
