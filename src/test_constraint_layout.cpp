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

#include <iostream>
#include <memory>
#include "view/ConstraintLayout.h"
#include "cassowary/ConstraintWidget.h"
#include "graphics/drawable/AnimatedVectorDrawable.h"
#include "graphics/drawable/VectorDrawable.h"
#include "animation/ObjectAnimator.h"
#include "animation/PropertyRegistry.h"
#include "utils/SystemClock.h"
#include "os/Looper.h"


using namespace setu::view;

void check(bool condition, const std::string& message) {
    if (condition) {
        std::cout << "  [PASS] " << message << std::endl;
    } else {
        std::cout << "  [FAIL] " << message << std::endl;
        exit(1);
    }
}

void check_eq(int actual, int expected, const std::string& name) {
    if (actual == expected) {
        std::cout << "  [PASS] " << name << " is " << expected << std::endl;
    } else {
        std::cout << "  [FAIL] " << name << " is " << actual << " (expected " << expected << ")" << std::endl;
        exit(1);
    }
}

int main() {
    std::cout << "=== Phase 1: ConstraintLayout View class (isolated) ===" << std::endl;

    auto layout = std::make_shared<ConstraintLayout>();
    
    auto v1 = std::make_shared<View>();
    v1->setLayoutParams(std::make_shared<ConstraintLayout::LayoutParams>(100, 50));
    
    auto v2 = std::make_shared<View>();
    v2->setLayoutParams(std::make_shared<ConstraintLayout::LayoutParams>(200, 50));

    layout->addConstrainedChild(v1);
    layout->addConstrainedChild(v2);

    auto w1 = layout->getWidget(v1.get());
    auto w2 = layout->getWidget(v2.get());
    auto wRoot = layout->getWidget(layout.get()); // wait, layout widget isn't a child
    
    // We can just access the widget if we expose getRootWidget, but it's okay, we can just connect them to each other for now, or expose getRootWidget.
    // Actually, w1->getParent() returns the root.
    auto rootWidget = w1->getParent();

    // v1 left to root left
    w1->mLeft.connect(rootWidget->getAnchor(setu::cassowary::ConstraintAnchor::Type::LEFT), 0);
    // v2 left to v1 right
    w2->mLeft.connect(w1->getAnchor(setu::cassowary::ConstraintAnchor::Type::RIGHT), 0);

    // Call onMeasure
    int widthSpec = View::makeMeasureSpec(1000, View::MEASURE_SPEC_EXACTLY);
    int heightSpec = View::makeMeasureSpec(1000, View::MEASURE_SPEC_EXACTLY);
    layout->measure(widthSpec, heightSpec);
    
    // Call onLayout
    layout->layout(0, 0, 1000, 1000);

    // Check v1
    check_eq(v1->getLeft(), 0, "v1 left");
    check_eq(v1->getWidth(), 100, "v1 width");
    
    // Check v2
    check_eq(v2->getLeft(), 100, "v2 left");
    check_eq(v2->getWidth(), 200, "v2 width");

    std::cout << "=== All Phase 1 tests passed! ===" << std::endl;
    
    std::cout << "\n=== Phase 3: AnimatedVectorDrawable Test ===" << std::endl;
    // 1. Setup Looper
    if (!setu::os::Looper::myLooper()) {
        setu::os::Looper::prepareMainLooper();
    }

    // 2. Create VectorDrawable with a path
    auto vd = std::make_shared<setu::graphics::VectorDrawable>();
    auto path = std::make_shared<setu::graphics::VPath>();
    path->name = "my_path";
    path->trimPathEnd = 0.0f; // initial value
    vd->getRootGroup()->paths.push_back(path);

    // 3. Create AnimatedVectorDrawable
    auto avd = std::make_shared<setu::graphics::AnimatedVectorDrawable>(vd);

    // 4. Create ObjectAnimator to animate trimPathEnd from 0 to 1
    setu::animation::PropertyRegistry::init();
    auto animator = setu::animation::ObjectAnimator::ofFloat(vd->getTargetByName("my_path"), "", "trimPathEnd", {0.0f, 1.0f});
    animator->setDuration(1000); // 1 second

    // 5. Register animator with AVD
    avd->registerAnimator(animator);

    // 6. Start the animation
    std::cout << "Starting animation..." << std::endl;
    avd->start();
    check(avd->isRunning(), "AVD is running after start()");

    std::cout << "Initial trimPathEnd: " << path->trimPathEnd << std::endl;
    
    long long startTime = setu::uptimeMillis();
    for (int i = 1; i <= 5; ++i) {
        long long tickTime = startTime + (i * 200);
        setu::animation::ValueAnimator::doFrame(tickTime * 1000000LL);
        std::cout << "Frame " << i << " (+" << (i*200) << "ms) trimPathEnd: " << path->trimPathEnd << std::endl;
    }
    
    // Stop the animation
    avd->stop();
    check(!avd->isRunning(), "AVD is stopped after stop()");

    return 0;
}
#include "ui/WindowManager.h"

void WindowManager::wakeLooper(long long delayMs) {}
