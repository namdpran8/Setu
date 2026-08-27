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

#include <vector>

#include "DrawableContainer.h"

namespace setu {
namespace graphics {

// android.graphics.drawable.StateListDrawable: the <selector> element.
//
// This is how nearly every interactive widget on a pre-Material device knows what
// it looks like. A Button's background is a selector over pressed / focused /
// disabled / default; a list row's is a selector over pressed / activated; a
// checkbox's is a selector over checked. Without it, a real APK renders every
// widget in its resting appearance and never reacts to touch, which is the
// single most obvious "this is not a real device" tell after wrong text layout.
//
// The matching rule is first-match-wins in document order, so a selector is
// written most-specific first and ends with a stateless <item> that acts as the
// default. The class itself is only the lookup; DrawableContainer does the work
// of actually showing the chosen child.
class StateListDrawable : public DrawableContainer {
public:
    StateListDrawable() = default;

    // `stateSet` is the <item>'s requirement list in StateSet form: positive for
    // state_x="true", negative for state_x="false", empty for a wildcard item.
    // A null drawable is ignored, as in AOSP.
    void addState(const std::vector<int>& stateSet, DrawablePtr drawable);

    // Always true, even for a one-item selector: an owner has to keep pushing
    // state in, because whether a given state matches is not knowable from here.
    bool isStateful() const override { return true; }

    int getStateCount() const { return getChildCount(); }
    const std::vector<int>& getStateSet(int index) const;
    Drawable* getStateDrawable(int index) const { return getChild(index); }

    // Index of the first item whose requirements the given state satisfies, or -1.
    int indexOfStateSet(const std::vector<int>& stateSet) const;

protected:
    bool onStateChange(const std::vector<int>& stateSet) override;

private:
    // Parallel to the container's children: mStateSets[i] is what child i needs.
    std::vector<std::vector<int>> mStateSets;
};

} // namespace graphics
} // namespace setu
