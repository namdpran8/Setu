#pragma once

#include <cstdint>

#include "androidfw/ResourceTypes.h"

#include "../graphics/ColorStateList.h"

namespace setu {

class ResourceManager;
class Theme;

// Turns a compiled res/color resource into a graphics::ColorStateList.
//
// The sibling of DrawableInflater, and deliberately a separate class rather than
// a method on it: res/color/foo.xml and res/drawable/foo.xml both have a
// <selector> root but mean different things, and a caller always knows which one
// it wants. android:textColor wants a colour list and would be wrong to accept a
// <shape>; android:background wants a drawable and would be wrong to accept a
// bare colour list.
//
// A plain colour resource (@color/foo declared as #ff0000 in values/) is also a
// valid result: it inflates to a one-item constant list, exactly as AOSP's
// Resources.getColorStateList does. That is what lets every consumer hold a
// ColorStateList and stop caring whether the author wrote a literal or a
// selector.
class ColorStateListInflater {
public:
    // Inflates the colour list a resource ID points at. Returns nullptr when the
    // resource is not a colour list at all - a caller can then fall back to its
    // own default rather than paint the DEFAULT_COLOR red.
    static graphics::ColorStateListPtr inflate(ResourceManager* resManager, Theme* theme,
                                               uint32_t resId);

    // Inflates from a parser already sitting on the <selector> root START_TAG.
    // Consumes the element, leaving the parser on its END_TAG.
    static graphics::ColorStateListPtr inflateFromParser(android::ResXMLParser* parser,
                                                         ResourceManager* resManager,
                                                         Theme* theme);
};

} // namespace setu
