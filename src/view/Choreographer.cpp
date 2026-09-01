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

#include "Choreographer.h"
#include "../animation/ValueAnimator.h"
#include "../utils/SystemClock.h"
#include "../ui/WindowManager.h"

namespace setu {
namespace view {

void Choreographer::doFrame(std::shared_ptr<View> decorView, graphics::Canvas& canvas, int windowWidth, int windowHeight) {
    long long frameTimeNanos = setu::uptimeMillis() * 1000000LL;
    setu::animation::ValueAnimator::doFrame(frameTimeNanos);

    if (!decorView) return;

    // 1. Measure Pass
    int widthSpec = View::makeMeasureSpec(windowWidth, View::MEASURE_SPEC_EXACTLY);
    int heightSpec = View::makeMeasureSpec(windowHeight, View::MEASURE_SPEC_EXACTLY);
    decorView->measure(widthSpec, heightSpec);

    // 2. Layout Pass
    decorView->layout(0, 0, decorView->getMeasuredWidth(), decorView->getMeasuredHeight());
    WindowManager::dumpRootViewAfterLayout();

    // 3. Draw Pass (Update Display List)
    decorView->updateRenderNode();

    // 4. Render to screen
    // Clear screen
    canvas.drawColor(0xFFFFFFFF); // White background
    
    // Execute RenderNode display list
    canvas.drawRenderNode(decorView->getRenderNode());
}

} // namespace view
} // namespace setu
