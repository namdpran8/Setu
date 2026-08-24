#pragma once

#include <cstdint>
#include <memory>

#include "androidfw/ResourceTypes.h"

#include "../graphics/drawable/Drawable.h"

namespace setu {

class ResourceManager;
class Theme;

// Turns a compiled drawable resource into a graphics::Drawable.
//
// This is the other half of Phase 2: GradientDrawable knows how to paint a
// <shape>, and this is what reads one out of an APK. It sits in ui/ next to
// LayoutInflater on purpose - graphics/ stays free of any dependency on
// androidfw or ResourceManager, so the drawing code can be reasoned about (and
// unit-tested) without a loaded APK.
//
// Root elements that belong to later phases (<ripple>, <bitmap>, <nine-patch>)
// are recognised and logged rather than silently dropped, so a missing
// background in a real app names the phase that will fix it.
class DrawableInflater {
public:
    // Inflates the drawable a resource ID points at. Returns nullptr when the
    // resource is not a drawable, or is one of the kinds not yet supported.
    //
    // A fresh instance every call, deliberately: a View installs itself as its
    // background's Callback and pushes its own bounds in, so two Views sharing
    // one Drawable would fight over both. AOSP shares the immutable
    // ConstantState instead, which is the eventual answer, not a cache here.
    static graphics::DrawablePtr inflate(ResourceManager* resManager, Theme* theme, uint32_t resId);

    // Inflates from a parser already sitting on the drawable's root START_TAG.
    // Consumes the element, leaving the parser on its END_TAG - so a caller
    // walking a <selector> can carry on with the next item.
    static graphics::DrawablePtr inflateFromParser(android::ResXMLParser* parser,
                                                   ResourceManager* resManager,
                                                   Theme* theme);

private:
    static graphics::DrawablePtr inflateShape(android::ResXMLParser* parser,
                                              ResourceManager* resManager,
                                              Theme* theme);
    static graphics::DrawablePtr inflateColor(android::ResXMLParser* parser,
                                              ResourceManager* resManager,
                                              Theme* theme);
    static graphics::DrawablePtr inflateSelector(android::ResXMLParser* parser,
                                                 ResourceManager* resManager,
                                                 Theme* theme);
};

} // namespace setu
