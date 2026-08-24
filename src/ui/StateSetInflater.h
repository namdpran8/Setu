#pragma once

#include <vector>

#include "androidfw/ResourceTypes.h"

namespace setu {

class XmlAttrs;

// The <item> state-spec reader, shared between the drawable <selector> inflater
// and the colour <selector> inflater.
//
// Both read exactly the same grammar: android:state_pressed="true" and friends
// become a spec of +token/-token entries, an unrecognised state_* attribute
// becomes a synthetic token so the item keeps its requirement, and an item with
// no state attributes becomes the empty (wildcard) spec. Having one reader is
// what guarantees a colour selector and a drawable selector resolve identical
// state to identical items - the whole point of Phase 4 sharing this code
// rather than the colour path growing a second, subtly different copy.
//
// The parser must be sitting on the <item> element and `attrs` must be bound to
// that same parser; neither is advanced. Detection of unknown states needs the
// attribute name strings, so binary XML compiled without them yields only the
// framework states, which is the same limitation the drawable path already had.
std::vector<int> extractStateSet(const android::ResXMLParser* parser, const XmlAttrs& attrs);

} // namespace setu
