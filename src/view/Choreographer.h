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
#include "View.h"
#include "../graphics/Direct2DCanvas.h"
#include <memory>

namespace setu {
namespace view {

class Choreographer {
public:
    static Choreographer& getInstance() {
        static Choreographer instance;
        return instance;
    }

    void doFrame(std::shared_ptr<View> decorView, graphics::Direct2DCanvas& canvas, int windowWidth, int windowHeight);

private:
    Choreographer() = default;
};

} // namespace view
} // namespace setu
