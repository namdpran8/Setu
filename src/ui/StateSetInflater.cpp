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

#include "StateSetInflater.h"

#include <cstdint>
#include <string>

#include "XmlAttrs.h"
#include "androidfw/Util.h"
#include "../graphics/drawable/StateSet.h"
#include "../utils/Logger.h"

namespace setu {

namespace {

const char* const TAG = "StateSetInflater";

std::string attributeName(const android::ResXMLParser* parser, size_t index) {
    size_t len = 0;
    const char16_t* name16 = parser->getAttributeName(index, &len);
    if (!name16 || len == 0) return std::string();
    return android::util::Utf16ToUtf8(android::StringPiece16(name16, len));
}

bool isKnownStateName(const std::string& name) {
    for (size_t i = 0; i < graphics::StateSet::ATTR_COUNT; ++i) {
        if (name == graphics::StateSet::ATTRS[i].name) return true;
    }
    return false;
}

// A stable token for a state attribute Windroid has never heard of. Framework
// and app resource IDs all carry a non-zero package byte, so they are every one
// of them >= 0x01000000 - which is why folding a name hash into the low 24 bits
// cannot collide with a real attribute ID, or with the constants in StateSet.h.
int unknownStateToken(const std::string& name) {
    uint32_t hash = 2166136261u;   // FNV-1a
    for (char c : name) {
        hash ^= (uint32_t)(unsigned char)c;
        hash *= 16777619u;
    }
    const int token = (int)(hash & 0x00FFFFFFu);
    return token != 0 ? token : 1;
}

} // namespace

std::vector<int> extractStateSet(const android::ResXMLParser* parser, const XmlAttrs& attrs) {
    std::vector<int> states;

    for (size_t i = 0; i < graphics::StateSet::ATTR_COUNT; ++i) {
        const graphics::StateSet::Attr& attr = graphics::StateSet::ATTRS[i];
        if (!attrs.has(attr.name)) continue;
        states.push_back(attrs.getBool(attr.name, false) ? attr.state : -attr.state);
    }

    // AOSP treats *every* attribute on an <item> except the value-carrying ones
    // (drawable and id for a drawable selector; color, alpha and lStar for a
    // colour one) as a state requirement, framework-declared or not - Material's
    // app:state_dragged, for instance. Dropping an unknown state would be the
    // wrong kind of wrong: the item would lose a requirement and start matching
    // views that are not in that state, so a dragged-card background would paint
    // on every card. Unknown states therefore get a token of their own. View
    // never reports it, so a must-match requirement can never be satisfied and a
    // must-not-match one always is - exactly what a real device does with a state
    // its View subclass never sets.
    //
    // Detection needs the attribute's name, so this finds nothing in binary XML
    // compiled without name strings. The prefix test is what keeps a stray
    // non-state attribute from being invented into a requirement; every state
    // attribute in the framework, and every one an app declares by convention, is
    // named state_*. It is also what makes this reader safe to share with the
    // colour path, whose value attributes (color, alpha, lStar) cannot match it.
    const size_t count = parser->getAttributeCount();
    for (size_t i = 0; i < count; ++i) {
        const std::string name = attributeName(parser, i);
        if (name.rfind("state_", 0) != 0) continue;
        if (isKnownStateName(name)) continue;

        const int token = unknownStateToken(name);
        const int type = parser->getAttributeDataType(i);
        // A non-inline value here is a reference this cannot resolve (XmlAttrs only
        // reads the android namespace, and a custom state is in the app's). Treated
        // as must-match, which is unsatisfiable - the same outcome as the far more
        // common state_x="true" spelling.
        const bool wanted = (type < android::Res_value::TYPE_FIRST_INT ||
                             type > android::Res_value::TYPE_LAST_INT) ||
                            parser->getAttributeData(i) != 0;
        states.push_back(wanted ? token : -token);
        Logger::d(TAG, "<item> requires unknown state " + name +
                           (wanted ? "=true; it can never match" : "=false; always satisfied"));
    }

    return states;
}

} // namespace setu
