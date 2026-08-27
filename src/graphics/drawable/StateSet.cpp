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

#include "StateSet.h"

namespace setu {
namespace graphics {
namespace StateSet {

const Attr ATTRS[] = {
    { "state_focused",         STATE_FOCUSED },
    { "state_window_focused",  STATE_WINDOW_FOCUSED },
    { "state_enabled",         STATE_ENABLED },
    { "state_checkable",       STATE_CHECKABLE },
    { "state_checked",         STATE_CHECKED },
    { "state_selected",        STATE_SELECTED },
    { "state_active",          STATE_ACTIVE },
    { "state_single",          STATE_SINGLE },
    { "state_first",           STATE_FIRST },
    { "state_middle",          STATE_MIDDLE },
    { "state_last",            STATE_LAST },
    { "state_pressed",         STATE_PRESSED },
    { "state_activated",       STATE_ACTIVATED },
    { "state_accelerated",     STATE_ACCELERATED },
    { "state_hovered",         STATE_HOVERED },
    { "state_drag_can_accept", STATE_DRAG_CAN_ACCEPT },
    { "state_drag_hovered",    STATE_DRAG_HOVERED },
};

const size_t ATTR_COUNT = sizeof(ATTRS) / sizeof(ATTRS[0]);

bool matches(const std::vector<int>& spec, const std::vector<int>& set) {
    const size_t specSize = spec.size();
    const size_t setSize = set.size();

    for (size_t i = 0; i < specSize; ++i) {
        int wanted = spec[i];
        // AOSP's sets are fixed-capacity arrays padded with zeroes, so a zero
        // means "no more requirements". Ours are exact-sized vectors and should
        // never contain one, but a stray zero must not be treated as a state.
        if (wanted == 0) return true;

        bool mustMatch;
        if (wanted > 0) {
            mustMatch = true;
        } else {
            mustMatch = false;
            wanted = -wanted;
        }

        bool found = false;
        for (size_t j = 0; j < setSize; ++j) {
            const int state = set[j];
            if (state == 0) {
                // End of the states actually held. A must-match requirement can
                // no longer be satisfied; a must-not-match one is satisfied by
                // the absence, so move on to the next requirement.
                if (mustMatch) return false;
                break;
            }
            if (state == wanted) {
                if (mustMatch) {
                    found = true;
                    break;
                }
                // Present, but the item demanded its absence.
                return false;
            }
        }

        if (mustMatch && !found) return false;
    }
    return true;
}

const char* name(int state) {
    const int token = state < 0 ? -state : state;
    for (size_t i = 0; i < ATTR_COUNT; ++i) {
        if (ATTRS[i].state == token) return ATTRS[i].name;
    }
    return "state_?";
}

std::string describe(const std::vector<int>& set) {
    std::string out = "[";
    for (size_t i = 0; i < set.size(); ++i) {
        if (i > 0) out += ' ';
        if (set[i] < 0) out += '-';
        out += name(set[i]);
    }
    out += ']';
    return out;
}

} // namespace StateSet
} // namespace graphics
} // namespace setu
