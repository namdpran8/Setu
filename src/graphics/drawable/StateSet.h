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

#include <cstddef>
#include <string>
#include <vector>

namespace setu {
namespace graphics {

// android.util.StateSet, plus the drawable-state attributes it works with.
//
// A state set is a flat list of state tokens: positive means "this state must be
// set", negative means "must not be set". So
//
//   <item android:state_pressed="true"  .../>   ->  { +STATE_PRESSED }
//   <item android:state_enabled="false" .../>   ->  { -STATE_ENABLED }
//   <item .../>                                 ->  { }  (the wildcard)
//
// and the empty list matches anything, which is what makes the last item of a
// <selector> the default. Order within a set never matters; matching searches.
//
// The values below are the real framework attribute IDs, but nothing depends on
// that. They are private tokens: DrawableInflater looks its attributes up by
// *name* through XmlAttrs and records the token from this header, and
// View::onCreateDrawableState() builds its vector from the same header. The two
// halves therefore cannot disagree, even if a constant here were wrong.
namespace StateSet {

// Declared in the order AOSP's attrs.xml declares them, which is also the order
// View reports them in - purely so a logged state set reads the same way it does
// in a real device's hierarchy dump.
constexpr int STATE_FOCUSED          = 0x0101009c;
constexpr int STATE_WINDOW_FOCUSED   = 0x0101009d;
constexpr int STATE_ENABLED          = 0x0101009e;
constexpr int STATE_CHECKABLE        = 0x0101009f;
constexpr int STATE_CHECKED          = 0x010100a0;
constexpr int STATE_SELECTED         = 0x010100a1;
constexpr int STATE_ACTIVE           = 0x010100a2;
constexpr int STATE_SINGLE           = 0x010100a3;
constexpr int STATE_FIRST            = 0x010100a4;
constexpr int STATE_MIDDLE           = 0x010100a5;
constexpr int STATE_LAST             = 0x010100a6;
constexpr int STATE_PRESSED          = 0x010100a7;
constexpr int STATE_ACTIVATED        = 0x010102fe;
constexpr int STATE_ACCELERATED      = 0x0101031b;
constexpr int STATE_HOVERED          = 0x01010367;
constexpr int STATE_DRAG_CAN_ACCEPT  = 0x01010368;
constexpr int STATE_DRAG_HOVERED     = 0x01010369;

// The XML attribute name for each token, so the inflater can recognise a
// <selector> item's states and a log line can name them back.
struct Attr {
    const char* name;   // without the "android:" prefix
    int state;
};
extern const Attr ATTRS[];
extern const size_t ATTR_COUNT;

// android.util.StateSet.stateSetMatches. `spec` is what an <item> requires;
// `set` is the state the owning View is actually in.
//
// Note what an *empty* `set` does: every must-not-match requirement in `spec`
// passes, so a `<item android:state_enabled="false">` matches a View that has
// not reported its state yet. That is why View::setBackground() pushes
// getDrawableState() in immediately - otherwise a stock disabled-first selector
// would render every button greyed out.
bool matches(const std::vector<int>& spec, const std::vector<int>& set);

// The attribute name for a token, or "state_?" when it is not one of ours.
// Negative tokens are accepted and reported without their sign.
const char* name(int state);

// A state set rendered for a log line, e.g.
// "[state_window_focused state_enabled -state_pressed]". Worth having: when a
// selector picks the wrong item the state vector is the first thing to look at,
// and a list of eight-digit hex IDs says nothing.
std::string describe(const std::vector<int>& set);

} // namespace StateSet

} // namespace graphics
} // namespace setu
