#include "../graphics/drawable/InsetDrawable.h"
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
#include "DrawableInflater.h"
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "StateSetInflater.h"
#include "Theme.h"
#include "WindowManager.h"
#include "XmlAttrs.h"
#include "../dex/ResourceManager.h"
#include "../graphics/BitmapFactory.h"
#include "../graphics/drawable/BitmapDrawable.h"
#include "../graphics/drawable/ColorDrawable.h"
#include "../graphics/drawable/GradientDrawable.h"
#include "../graphics/drawable/NinePatchDrawable.h"
#include "../graphics/drawable/RippleDrawable.h"
#include "../graphics/drawable/LayerDrawable.h"
#include "../graphics/drawable/LevelListDrawable.h"
#include "../graphics/drawable/VectorDrawable.h"
#include "../graphics/drawable/AnimatedVectorDrawable.h"
#include "../animation/AnimatorInflater.h"
#include "../animation/AnimatorSet.h"
#include "../animation/ObjectAnimator.h"
#include "../graphics/drawable/StateListDrawable.h"
#include "../graphics/drawable/StateSet.h"
#include "../view/Gravity.h"
#include "include/utils/SkParsePath.h"
#include "../utils/Logger.h"
namespace setu {
namespace {
const char* const TAG = "DrawableInflater";
// @android:id/mask: the well-known ID a <ripple> gives the child that shapes it.
// Hard-coded rather than looked up by name, because it is a *public* framework ID
// and therefore frozen for good, and because this has to keep working with no
// framework-res loaded - which is exactly when a name lookup would fail.
constexpr uint32_t ANDROID_ID_MASK = 0x0102002e;
bool endsWith(const std::string& str, const char* suffix) {
    const std::string s(suffix);
    return str.size() >= s.size() && str.compare(str.size() - s.size(), s.size(), s) == 0;
}
// The screen density in DPI, which is the unit both bitmap drawables want. Note the
// conversion: WindowManager stores a float scale factor (3.0 for xxhdpi), and the
// drawables want 480. Passing the scale straight through would scale every image by
// 1/160th of its size.
int displayDensityDpi() {
    const float scale = WindowManager::getDensity();
    if (scale <= 0.0f) return graphics::Bitmap::DENSITY_DEFAULT;
    const int dpi = (int)std::lround(scale * (float)graphics::Bitmap::DENSITY_DEFAULT);
    return dpi > 0 ? dpi : graphics::Bitmap::DENSITY_DEFAULT;
}
// The density a resource's own configuration declares, as a Bitmap density.
//
// Two zeroes that mean opposite things meet here, which is the whole reason this is
// a function: ResTable_config::DENSITY_DEFAULT is 0 and means "no density qualifier
// on the directory", i.e. res/drawable/, i.e. mdpi. Bitmap::DENSITY_NONE is also 0
// and means "unknown, do not scale". Mapping the first onto the second would leave
// every unqualified drawable unscaled.
int sourceDensityFromConfig(uint16_t configDensity, int targetDensityDpi) {
    switch (configDensity) {
        case android::ResTable_config::DENSITY_DEFAULT:
            // res/drawable/, no qualifier. AOSP's ResourcesImpl substitutes the
            // baseline here, so the asset scales up on a dense screen.
            return graphics::Bitmap::DENSITY_DEFAULT;
        case android::ResTable_config::DENSITY_NONE:
            // drawable-nodpi: authored in real pixels, never resampled.
            return graphics::Bitmap::DENSITY_NONE;
        case android::ResTable_config::DENSITY_ANY:
            // drawable-anydpi, which is meant for vectors. A raster found there has
            // no declared size, so treating it as already-correct for this screen is
            // the only reading that does not invent a scale factor.
            return targetDensityDpi;
        default:
            return (int)configDensity;
    }
}
// Everything one image file yields. The chunk is kept alongside the pixels because
// only the decode can recover it - WIC drops unknown PNG chunks, so it is harvested
// from the encoded bytes and cannot be read back off the Bitmap later.
struct DecodedImage {
    std::shared_ptr<graphics::Bitmap> bitmap;
    std::vector<uint8_t> ninePatchChunk;
    bool pathSaysNinePatch = false;
    int targetDensityDpi = graphics::Bitmap::DENSITY_DEFAULT;
};
// Resolves a drawable resource ID to a file, opens it out of the APK it was selected
// from, and decodes it. False on any failure, having logged which one.
bool decodeImageResource(ResourceManager* resManager, Theme* theme, uint32_t resId,
                         DecodedImage& out) {
    if (!resManager || resId == 0) return false;
    auto* assets = resManager->getAssetManager();
    if (!assets) return false;
    const std::string path = resManager->getResourceFilePath(resId);
    if (path.empty()) {
        Logger::w(TAG, "Drawable 0x" + std::to_string(resId) + " is not a file");
        return false;
    }
    if (endsWith(path, ".xml")) {
        // android:src naming another XML drawable. Legal to write, meaningless to
        // both classes, and diagnosed here rather than as a puzzling decode failure
        // twenty lines down.
        Logger::w(TAG, "android:src must name an image file, not " + path);
        return false;
    }
    out.targetDensityDpi = displayDensityDpi();
    out.pathSaysNinePatch = endsWith(path, ".9.png");
    // A second lookup of the same ID. getResourceFilePath resolves aliases and hands
    // back the path but keeps neither the cookie nor the configuration, and both are
    // needed here: the cookie to open the file out of the *right* APK (the app and
    // framework-res both ship res/drawable-hdpi paths), and the config for the
    // density the directory name declares.
    int sourceDensity = graphics::Bitmap::DENSITY_DEFAULT;
    android::ApkAssetsCookie cookie = android::kInvalidCookie;
    if (auto res = assets->GetResource(resId); res.has_value()) {
        android::AssetManager2::SelectedValue val = res.value();
        if (resManager->resolveValue(val, theme)) {
            cookie = val.cookie;
            sourceDensity = sourceDensityFromConfig(val.config.density, out.targetDensityDpi);
        }
    }
    // Falling back to the path-only overload rather than bailing out: it searches
    // every loaded APK in reverse order, which is what openXml does and is right far
    // more often than it is wrong.
    std::unique_ptr<android::Asset> asset =
        cookie == android::kInvalidCookie
            ? assets->OpenNonAsset(path, android::Asset::ACCESS_BUFFER)
            : assets->OpenNonAsset(path, cookie, android::Asset::ACCESS_BUFFER);
    if (!asset) {
        Logger::w(TAG, "Could not open image asset: " + path);
        return false;
    }
    graphics::BitmapFactory::Options options;
    options.inDensity = sourceDensity;
    out.bitmap = graphics::BitmapFactory::decodeAsset(asset.get(), &options);
    if (!out.bitmap) {
        // BitmapFactory has already logged the specific reason, which for a .webp on
        // a machine without the Store codec is the one worth reading.
        Logger::w(TAG, "Could not decode " + path);
        return false;
    }
    out.ninePatchChunk = std::move(options.outNinePatchChunk);
    return true;
}
// android:alpha is a 0..1 float on a drawable, unlike the 0..255 int setAlpha takes.
float clampUnit(float alpha) {
    return alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);
}
int alphaFloatToInt(float alpha) {
    return (int)std::lround(clampUnit(alpha) * 255.0f);
}
// Wraps an already-decoded image, target density applied. Takes the DecodedImage
// rather than a resource ID so that the raw-file path can decide which of the two
// classes to build *after* decoding, instead of decoding twice to find out.
//
// Both consume image.bitmap.
std::shared_ptr<graphics::BitmapDrawable> makeBitmapDrawable(DecodedImage& image) {
    auto drawable = std::make_shared<graphics::BitmapDrawable>(std::move(image.bitmap));
    drawable->setTargetDensity(image.targetDensityDpi);
    return drawable;
}
// Returns a drawable even when the chunk is missing or unusable: the class documents
// that case as a plain stretch, which is a better background than none. `what` names
// the resource in the warning that says so.
std::shared_ptr<graphics::NinePatchDrawable> makeNinePatchDrawable(DecodedImage& image,
                                                                  const std::string& what) {
    auto drawable = std::make_shared<graphics::NinePatchDrawable>(std::move(image.bitmap),
                                                                 image.ninePatchChunk);
    // After the chunk, not before: setTargetDensity rescales the padding the chunk
    // supplied, so it has to see a parsed chunk to have anything to rescale.
    drawable->setTargetDensity(image.targetDensityDpi);
    if (!drawable->hasValidChunk()) {
        Logger::w(TAG, what + " has no usable npTc chunk; it will stretch as a plain bitmap");
    }
    return drawable;
}
// Per-channel mean of two ARGB colours.
uint32_t meanColor(uint32_t x, uint32_t y) {
    uint32_t result = 0;
    for (int shift = 0; shift <= 24; shift += 8) {
        const uint32_t channel = (((x >> shift) & 0xFF) + ((y >> shift) & 0xFF)) / 2u;
        result |= channel << shift;
    }
    return result;
}
// Mean of a three-stop ramp: the middle stop covers half the run, the ends a
// quarter each.
uint32_t meanColor3(uint32_t start, uint32_t center, uint32_t end) {
    uint32_t result = 0;
    for (int shift = 0; shift <= 24; shift += 8) {
        const uint32_t channel = (((start >> shift) & 0xFF) +
                                  ((center >> shift) & 0xFF) * 2u +
                                  ((end >> shift) & 0xFF)) / 4u;
        result |= channel << shift;
    }
    return result;
}
// The drawable an android:drawable-style attribute names.
//
// This branches on the resolved *type* rather than just taking the resource ID,
// because android:drawable="#80000000" is legal and means a flat colour - which
// has no resource ID at all and would otherwise silently become "no drawable".
graphics::DrawablePtr drawableFromAttr(const XmlAttrs& attrs, const char* name,
                                      ResourceManager* resManager, Theme* theme) {
    android::AssetManager2::SelectedValue val;
    if (!attrs.getValue(name, val)) return nullptr;
    if (val.type >= android::Res_value::TYPE_FIRST_INT &&
        val.type <= android::Res_value::TYPE_LAST_INT) {
        return std::make_shared<graphics::ColorDrawable>(val.data);
    }
    if (val.resid != 0) {
        return DrawableInflater::inflate(resManager, theme, val.resid);
    }
    return nullptr;
}
// The drawable a container's <item> wraps when it is a nested element rather than
// an android:drawable reference. Leaves the parser on the <item>'s END_TAG.
graphics::DrawablePtr inflateFirstChild(android::ResXMLParser* parser,
                                       ResourceManager* resManager, Theme* theme) {
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            return nullptr;   // </item> already: an item with nothing in it
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        graphics::DrawablePtr child =
            DrawableInflater::inflateFromParser(parser, resManager, theme);
        // inflateFromParser left the parser on the child's END_TAG, so from here
        // skipCurrentElement walks out to </item> - the caller's loop then resumes
        // exactly where it would have if android:drawable had named the drawable.
        skipCurrentElement(parser);
        return child;
    }
    return nullptr;
}
// One <item> of a <selector>. Consumes the element either way.
void inflateSelectorItem(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme,
                         graphics::StateListDrawable& selector) {
    // Attributes have to be read before anything advances the parser, because
    // XmlAttrs reads through it live rather than taking a copy. AOSP's
    // StateListDrawable carries a note about the same ordering constraint.
    const XmlAttrs attrs(parser, resManager, theme);
    const std::vector<int> states = extractStateSet(parser, attrs);
    graphics::DrawablePtr child = drawableFromAttr(attrs, "drawable", resManager, theme);
    if (child) {
        skipCurrentElement(parser);
    } else {
        // Either android:drawable was absent and the drawable is a nested element,
        // or it named something not supported yet. Nested is the only place left to
        // look, and looking there also consumes the element when it is empty.
        child = inflateFirstChild(parser, resManager, theme);
    }
    if (!child) {
        // Dropping the item rather than adding a null child is the right
        // degradation: the selector falls through to whatever item comes next, which
        // is usually the default one, instead of painting nothing in that state.
        Logger::d(TAG, "<selector> item " + graphics::StateSet::describe(states) +
                           " has no usable drawable; skipped");
        return;
    }
    selector.addState(states, std::move(child));
}
// One <item> of a <ripple>. Consumes the element either way, and reports the
// layer's android:id so the caller can tell the mask from the content.
graphics::DrawablePtr inflateRippleItem(android::ResXMLParser* parser, ResourceManager* resManager,
                                        Theme* theme, uint32_t& outId) {
    // Same ordering constraint as inflateSelectorItem: XmlAttrs reads through the
    // parser live, so every attribute has to come off it before anything advances.
    const XmlAttrs attrs(parser, resManager, theme);
    outId = attrs.getResourceId("id");
    graphics::DrawablePtr child = drawableFromAttr(attrs, "drawable", resManager, theme);
    if (child) {
        skipCurrentElement(parser);
    } else {
        child = inflateFirstChild(parser, resManager, theme);
    }
    // Unread: android:left/top/right/bottom, android:width/height and
    // android:gravity, which inset or size a layer within the stack. They need a
    // real layer stack to mean anything, and a <ripple> that uses them is rare
    // enough that AOSP's own Material resources never do.
    return child;
}
// The roadmap item that will make a given root element work, for the log line.
const char* phaseFor(const std::string& tag) {
    if (tag == "vector" || tag == "animated-vector") return "not yet on the roadmap (vector drawables)";
    if (tag == "animated-selector") {
        // A different class, not a variant of <ripple>: AnimatedStateListDrawable
        // cross-fades or plays an AnimationDrawable *between* selector items, which
        // needs frame-by-frame drawables rather than a touch effect.
        return "not yet on the roadmap (AnimatedStateListDrawable)";
    }
    if (tag == "layer-list" || tag == "level-list" || tag == "transition") {
        return "not yet on the roadmap (drawable containers)";
    }
    return "not yet supported";
}
} // namespace
graphics::DrawablePtr DrawableInflater::inflate(ResourceManager* resManager, Theme* theme,
                                               uint32_t resId) {
    if (!resManager || resId == 0) return nullptr;
    const std::string path = resManager->getResourceFilePath(resId);
    if (path.empty()) {
        // Not a file. Most often @color/foo, which is a perfectly good background:
        // a flat colour is exactly what ColorDrawable is for.
        auto* assets = resManager->getAssetManager();
        if (!assets) return nullptr;
        auto res = assets->GetResource(resId);
        if (!res.has_value()) return nullptr;
        android::AssetManager2::SelectedValue val = res.value();
        if (!resManager->resolveValue(val, theme)) return nullptr;
        if (val.type >= android::Res_value::TYPE_FIRST_INT &&
            val.type <= android::Res_value::TYPE_LAST_INT) {
            return std::make_shared<graphics::ColorDrawable>(val.data);
        }
        Logger::d(TAG, "Resource 0x" + std::to_string(resId) +
                           " is neither a file nor a colour. Type=" + std::to_string(val.type));
        return nullptr;
    }
    if (!endsWith(path, ".xml")) {
        // A raw image referenced directly: @drawable/icon landing on
        // res/drawable-hdpi/icon.png, with no <bitmap> wrapper to carry attributes.
        return inflateImageFile(resManager, theme, resId, path);
    }
    auto tree = resManager->openXml(resId);
    if (!tree) return nullptr;
    android::ResXMLParser::event_code_t event;
    while ((event = tree->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            return inflateFromParser(tree.get(), resManager, theme);
        }
    }
    Logger::w(TAG, "No root element in drawable XML: " + path);
    return nullptr;
}
graphics::DrawablePtr DrawableInflater::inflateFromParser(android::ResXMLParser* parser,
                                                         ResourceManager* resManager,
                                                         Theme* theme) {
    if (!parser) return nullptr;
    const std::string tag = elementName(parser);
    if (tag.empty()) return nullptr;
    if (tag == "shape") {
        return inflateShape(parser, resManager, theme);
    }
    if (tag == "color") {
        return inflateColor(parser, resManager, theme);
    }
    if (tag == "selector") {
        return inflateSelector(parser, resManager, theme);
    }
    if (tag == "ripple") {
        return inflateRipple(parser, resManager, theme);
    }
    if (tag == "bitmap") {
        return inflateBitmap(parser, resManager, theme);
    }
    if (tag == "nine-patch") {
        return inflateNinePatch(parser, resManager, theme);
    }
    if (tag == "inset") {
        return inflateInset(parser, resManager, theme);
    }
    if (tag == "layer-list") {
        return inflateLayerList(parser, resManager, theme);
    }
    if (tag == "level-list") {
        return inflateLevelList(parser, resManager, theme);
    }
    if (tag == "vector") {
        return inflateVector(parser, resManager, theme);
    }
    if (tag == "animated-vector") {
        const XmlAttrs attrs(parser, resManager, theme);
        uint32_t drawableId = attrs.getResourceId("drawable");
        graphics::DrawablePtr vectorChild = nullptr;
        
        if (drawableId != 0) {
            vectorChild = inflate(resManager, theme, drawableId);
        } else {
            // Check for inline drawable
            // For now, if no ID, we check if there's a child <aapt:attr name="android:drawable">
            // This is a simplified fallback.
        }
        
        auto avd = std::make_shared<graphics::AnimatedVectorDrawable>(std::dynamic_pointer_cast<graphics::VectorDrawable>(vectorChild));
        
        int depth = 1;
        android::ResXMLParser::event_code_t event;
        while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
               event != android::ResXMLParser::END_DOCUMENT) {
            if (event == android::ResXMLParser::END_TAG) {
                depth--;
                if (depth == 0) break;
            } else if (event == android::ResXMLParser::START_TAG) {
                if (depth == 1) {
                    std::string childTag = elementName(parser);
                    if (childTag == "target") {
                        const XmlAttrs targetAttrs(parser, resManager, theme);
                        std::string targetName = targetAttrs.getString("name");
                        uint32_t animId = targetAttrs.getResourceId("animation");
                        
                        if (!targetName.empty() && animId != 0) {
                            auto anim = animation::AnimatorInflater::loadAnimator(resManager, theme, animId);
                            if (anim && vectorChild) {
                                auto targetPtr = std::dynamic_pointer_cast<graphics::VectorDrawable>(vectorChild)->getTargetByName(targetName);
                                if (targetPtr) {
                                    anim->setTarget(targetPtr);
                                    avd->registerAnimator(anim);
                                } else {
                                    Logger::w(TAG, "Target not found in VectorDrawable: " + targetName);
                                }
                            }
                        }
                    }
                }
                depth++;
            }
        }
        return avd;
    }
    Logger::d(TAG, "<" + tag + "> is " + phaseFor(tag) + "; no background drawn");
    skipCurrentElement(parser);
    return nullptr;
}
graphics::DrawablePtr DrawableInflater::inflateColor(android::ResXMLParser* parser,
                                                    ResourceManager* resManager, Theme* theme) {
    const XmlAttrs attrs(parser, resManager, theme);
    uint32_t color = 0;
    const bool hasColor = readColor(attrs, "color", color);
    skipCurrentElement(parser);
    if (!hasColor) {
        Logger::w(TAG, "<color> without a readable android:color");
        return nullptr;
    }
    return std::make_shared<graphics::ColorDrawable>(color);
}
graphics::DrawablePtr DrawableInflater::inflateSelector(android::ResXMLParser* parser,
                                                       ResourceManager* resManager, Theme* theme) {
    auto selector = std::make_shared<graphics::StateListDrawable>();
    {
        const XmlAttrs attrs(parser, resManager, theme);
        // Both default false in AOSP, and both are visible in a pixel diff:
        // constantSize stops a widget resizing between states, and variablePadding
        // lets the padding follow the state instead of being the per-edge maximum
        // across every item - which is what keeps a button's label from shifting
        // when it is pressed.
        selector->setVariablePadding(attrs.getBool("variablePadding", false));
        selector->setConstantSize(attrs.getBool("constantSize", false));
        // android:enterFadeDuration / exitFadeDuration would cross-fade between
        // items. There is no frame clock yet, so the swap is instant - which is what
        // a selector without them does anyway, and that is nearly all of them.
        if (attrs.getInt("enterFadeDuration", 0) > 0 || attrs.getInt("exitFadeDuration", 0) > 0) {
            Logger::d(TAG, "<selector> fade durations ignored; state changes are instant");
        }
        // Also unread: android:dither, which means nothing at 32bpp, and
        // android:visible, because View pushes its own visibility into a background
        // the moment it installs one.
    }
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        // Every branch below consumes one whole element, so the parser is always
        // back at this level on the next turn - which makes any END_TAG seen here
        // </selector>. No depth counting needed, unlike <shape>, whose children are
        // leaves it deliberately reads without consuming.
        if (event == android::ResXMLParser::END_TAG) break;
        if (event != android::ResXMLParser::START_TAG) continue;
        const std::string tag = elementName(parser);
        if (tag == "item") {
            inflateSelectorItem(parser, resManager, theme, *selector);
        } else {
            Logger::d(TAG, "<selector> child <" + tag + "> is not an item; ignored");
            skipCurrentElement(parser);
        }
    }
    if (selector->getStateCount() == 0) {
        // An empty selector is stateful and paints nothing, so installing it would
        // cost a repaint on every touch just to draw a hole. Better no background.
        Logger::w(TAG, "<selector> has no usable items; no background drawn");
        return nullptr;
    }
    return selector;
}
graphics::DrawablePtr DrawableInflater::inflateRipple(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    graphics::ColorStateListPtr color;
    int radius = graphics::RippleDrawable::RADIUS_AUTO;
    {
        const XmlAttrs attrs(parser, resManager, theme);
        // A ColorStateList rather than a colour: the usual value is
        // ?colorControlHighlight, which resolves to a flat one, but a <ripple> whose
        // highlight varies by state is legal and getColorStateList reads both.
        color = attrs.getColorStateList("color");
        // RADIUS_AUTO is AOSP's own sentinel for this attribute, so an absent
        // android:radius means "grow to the bounds" with no extra branch here.
        radius = attrs.getDimensionPixelSize("radius", graphics::RippleDrawable::RADIUS_AUTO);
        if (attrs.has("effectColor")) {
            Logger::d(TAG, "<ripple> android:effectColor ignored: it only colours the "
                           "patterned (Android 12 sparkle) style, and this draws the solid "
                           "one every version falls back to");
        }
    }
    graphics::DrawablePtr content;
    graphics::DrawablePtr mask;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        // As in <selector>: every branch consumes one whole element, so an END_TAG
        // seen at this level is always </ripple>.
        if (event == android::ResXMLParser::END_TAG) break;
        if (event != android::ResXMLParser::START_TAG) continue;
        const std::string tag = elementName(parser);
        if (tag != "item") {
            Logger::d(TAG, "<ripple> child <" + tag + "> is not an item; ignored");
            skipCurrentElement(parser);
            continue;
        }
        uint32_t id = 0;
        graphics::DrawablePtr child = inflateRippleItem(parser, resManager, theme, id);
        if (!child) continue;
        if (id == ANDROID_ID_MASK) {
            mask = std::move(child);
        } else if (!content) {
            content = std::move(child);
        } else {
            // A genuine layer stack. Nothing here can composite one yet, so the
            // first layer is kept rather than the layers being drawn in whatever
            // order they happened to arrive.
            Logger::d(TAG, "<ripple> has more than one content layer; the extra ones are "
                           "dropped until drawable containers exist");
        }
    }
    if (!color) {
        // AOSP throws XmlPullParserException here, which aborts the inflation and
        // leaves the widget with no background at all. The content layer is the more
        // useful half of a <ripple> anyway, so it is kept on its own: the widget
        // looks right at rest and simply does not respond to touch.
        Logger::w(TAG, "<ripple> without android:color; drawing its content layer only");
        return content;
    }
    auto ripple = std::make_shared<graphics::RippleDrawable>(std::move(color));
    ripple->setMaxRadius(radius);
    if (content) ripple->setContent(std::move(content));
    if (mask) ripple->setMask(std::move(mask));
    // No emptiness check, unlike <selector>: a <ripple> with no children at all is
    // ?selectableItemBackground, which is a real and very common resource. It draws
    // nothing at rest and touch feedback when touched, which is the whole point.
    return ripple;
}
graphics::DrawablePtr DrawableInflater::inflateImageFile(ResourceManager* resManager, Theme* theme,
                                                        uint32_t resId, const std::string& path) {
    DecodedImage image;
    if (!decodeImageResource(resManager, theme, resId, image)) return nullptr;
    // The chunk is the authority. aapt compiles the marker border of a .9.png into an
    // npTc chunk and strips the border out of the pixels, so the chunk's presence is
    // the only thing that proves the divs describe these particular pixels.
    //
    // The suffix is the fallback, and it only changes the outcome when the two
    // disagree: a file named .9.png whose chunk did not survive the trip. That is a
    // broken build rather than a plain bitmap, and NinePatchDrawable is what says so -
    // it logs the missing chunk and then stretches, which is exactly what a
    // BitmapDrawable would have done without mentioning it.
    if (!image.ninePatchChunk.empty() || image.pathSaysNinePatch) {
        return makeNinePatchDrawable(image, path);
    }
    // Gravity stays at BitmapDrawable's FILL default. A directly-referenced image has
    // no android:gravity to read, and AOSP's createFromResourceStream builds exactly
    // this - which is why @drawable/icon used as a background stretches to the view
    // rather than sitting at its intrinsic size in the middle.
    return makeBitmapDrawable(image);
}
graphics::DrawablePtr DrawableInflater::inflateLayerList(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    auto layerDrawable = std::make_shared<graphics::LayerDrawable>();
    
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) break;
            --depth;
            continue;
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        
        if (depth == 0 && elementName(parser) == "item") {
            const XmlAttrs attrs(parser, resManager, theme);
            int left = attrs.getDimensionPixelOffset("left", 0);
            int top = attrs.getDimensionPixelOffset("top", 0);
            int right = attrs.getDimensionPixelOffset("right", 0);
            int bottom = attrs.getDimensionPixelOffset("bottom", 0);
            
            uint32_t drawableId = attrs.getResourceId("drawable");
            graphics::DrawablePtr childDrawable;
            
            if (drawableId != 0) {
                childDrawable = inflate(resManager, theme, drawableId);
            }
            
            // Inflate nested drawable
            int innerDepth = 0;
            while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
                   event != android::ResXMLParser::END_DOCUMENT) {
                if (event == android::ResXMLParser::END_TAG) {
                    if (innerDepth == 0) break;
                    --innerDepth;
                    continue;
                }
                if (event == android::ResXMLParser::START_TAG) {
                    if (innerDepth == 0 && !childDrawable) {
                        childDrawable = inflateFromParser(parser, resManager, theme);
                    } else {
                        skipCurrentElement(parser);
                    }
                    if (event != android::ResXMLParser::END_TAG) {
                        innerDepth++;
                    }
                }
            }
            
            if (childDrawable) {
                layerDrawable->addLayer(childDrawable, left, top, right, bottom);
            }
        } else {
            depth++;
        }
    }
    
    return layerDrawable;
}

graphics::DrawablePtr DrawableInflater::inflateLevelList(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    auto levelList = std::make_shared<graphics::LevelListDrawable>();
    
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) break;
            --depth;
            continue;
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        
        if (depth == 0 && elementName(parser) == "item") {
            const XmlAttrs attrs(parser, resManager, theme);
            int maxLevel = attrs.getInt("maxLevel", 0);
            int minLevel = attrs.getInt("minLevel", 0);
            
            uint32_t drawableId = attrs.getResourceId("drawable");
            graphics::DrawablePtr childDrawable;
            
            if (drawableId != 0) {
                childDrawable = inflate(resManager, theme, drawableId);
            }
            
            int innerDepth = 0;
            while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
                   event != android::ResXMLParser::END_DOCUMENT) {
                if (event == android::ResXMLParser::END_TAG) {
                    if (innerDepth == 0) break;
                    --innerDepth;
                    continue;
                }
                if (event == android::ResXMLParser::START_TAG) {
                    if (innerDepth == 0 && !childDrawable) {
                        childDrawable = inflateFromParser(parser, resManager, theme);
                    } else {
                        skipCurrentElement(parser);
                    }
                    if (event != android::ResXMLParser::END_TAG) {
                        innerDepth++;
                    }
                }
            }
            
            if (childDrawable) {
                levelList->addLevel(minLevel, maxLevel, childDrawable);
            }
        } else {
            depth++;
        }
    }
    
    return levelList;
}

void DrawableInflater::inflateVectorGroup(android::ResXMLParser* parser,
                                       ResourceManager* resManager, Theme* theme,
                                       graphics::VGroup* group) {
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) break;
            --depth;
            continue;
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        
        if (depth == 0) {
            std::string tag = elementName(parser);
            const XmlAttrs attrs(parser, resManager, theme);
            
            if (tag == "group") {
                auto childGroup = std::make_shared<graphics::VGroup>();
                if (attrs.has("name")) {
                    childGroup->name = attrs.getString("name");
                }
                childGroup->rotation = attrs.getFloat("rotation", 0.0f);
                childGroup->pivotX = attrs.getFloat("pivotX", 0.0f);
                childGroup->pivotY = attrs.getFloat("pivotY", 0.0f);
                childGroup->scaleX = attrs.getFloat("scaleX", 1.0f);
                childGroup->scaleY = attrs.getFloat("scaleY", 1.0f);
                childGroup->translateX = attrs.getFloat("translateX", 0.0f);
                childGroup->translateY = attrs.getFloat("translateY", 0.0f);
                
                inflateVectorGroup(parser, resManager, theme, childGroup.get());
                group->groups.push_back(std::move(childGroup));
                continue; // inflateVectorGroup advances the parser
            } else if (tag == "path") {
                auto vpath = std::make_shared<graphics::VPath>();
                if (attrs.has("name")) {
                    vpath->name = attrs.getString("name");
                }
                
                vpath->fillColor = 0;
                readColor(attrs, "fillColor", vpath->fillColor);
                vpath->fillAlpha = attrs.getFloat("fillAlpha", 1.0f);
                
                vpath->strokeColor = 0;
                readColor(attrs, "strokeColor", vpath->strokeColor);
                vpath->strokeWidth = attrs.getFloat("strokeWidth", 0.0f);
                vpath->strokeAlpha = attrs.getFloat("strokeAlpha", 1.0f);
                
                vpath->trimPathStart = attrs.getFloat("trimPathStart", 0.0f);
                vpath->trimPathEnd = attrs.getFloat("trimPathEnd", 1.0f);
                vpath->trimPathOffset = attrs.getFloat("trimPathOffset", 0.0f);
                
                int lineCap = attrs.getInt("strokeLineCap", 0); // 0=butt, 1=round, 2=square
                if (lineCap == 1) vpath->strokeLineCap = graphics::VPath::LineCap::ROUND;
                else if (lineCap == 2) vpath->strokeLineCap = graphics::VPath::LineCap::SQUARE;
                
                int lineJoin = attrs.getInt("strokeLineJoin", 0); // 0=miter, 1=round, 2=bevel
                if (lineJoin == 1) vpath->strokeLineJoin = graphics::VPath::LineJoin::ROUND;
                else if (lineJoin == 2) vpath->strokeLineJoin = graphics::VPath::LineJoin::BEVEL;
                
                int fillType = attrs.getInt("fillType", 0); // 0=nonZero, 1=evenOdd
                vpath->fillEvenOdd = (fillType != 0);
                
                std::string pathData = attrs.getString("pathData");
                if (!pathData.empty()) {
                    SkParsePath::FromSVGString(pathData.c_str(), &vpath->path);
                }
                
                group->paths.push_back(std::move(vpath));
            }
        }
        
        depth++;
    }
}

graphics::DrawablePtr DrawableInflater::inflateVector(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    auto vectorDrawable = std::make_shared<graphics::VectorDrawable>();
    
    const XmlAttrs attrs(parser, resManager, theme);
    float viewportWidth = attrs.getFloat("viewportWidth", 0.0f);
    float viewportHeight = attrs.getFloat("viewportHeight", 0.0f);
    vectorDrawable->setViewportSize(viewportWidth, viewportHeight);
    
    int width = attrs.getDimensionPixelSize("width", -1);
    int height = attrs.getDimensionPixelSize("height", -1);
    vectorDrawable->setIntrinsicSize(width, height);
    
    inflateVectorGroup(parser, resManager, theme, vectorDrawable->getRootGroup().get());
    
    return vectorDrawable;
}

graphics::DrawablePtr DrawableInflater::inflateBitmap(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    using BD = graphics::BitmapDrawable;
    // Every attribute has to come off the parser before anything advances it - the
    // same ordering constraint inflateSelectorItem carries, because XmlAttrs reads
    // through the parser live rather than taking a copy.
    const XmlAttrs attrs(parser, resManager, theme);
    const uint32_t srcId = attrs.getResourceId("src");
    const int gravity = attrs.getInt("gravity", view::Gravity::FILL);
    // TILE_MODE_UNDEFINED, not DISABLED: an absent android:tileModeY must not clear
    // what an android:tileMode on the same element just set. AOSP's inflate() makes
    // the same distinction with the same sentinel.
    const int tileMode = attrs.getInt("tileMode", BD::TILE_MODE_UNDEFINED);
    const int tileModeX = attrs.getInt("tileModeX", BD::TILE_MODE_UNDEFINED);
    const int tileModeY = attrs.getInt("tileModeY", BD::TILE_MODE_UNDEFINED);
    const bool antiAlias = attrs.getBool("antialias", false);
    const bool filter = attrs.getBool("filter", true);
    const float alpha = attrs.getFloat("alpha", 1.0f);
    const bool hasTint = attrs.has("tint");
    const bool hasMipMap = attrs.has("mipMap");
    const bool hasAutoMirrored = attrs.has("autoMirrored");
    skipCurrentElement(parser);
    if (srcId == 0) {
        // AOSP throws XmlPullParserException here. Nothing to draw either way, but a
        // warning beats an abort: the rest of the view hierarchy still inflates.
        Logger::w(TAG, "<bitmap> without a resolvable android:src; no background drawn");
        return nullptr;
    }
    DecodedImage image;
    if (!decodeImageResource(resManager, theme, srcId, image)) return nullptr;
    // Deliberately a BitmapDrawable even when the source carries an npTc chunk, which
    // is what a real device does: AOSP's BitmapDrawable holds a Bitmap and has no
    // chunk field at all, so a <bitmap> wrapping a .9.png stretches there too. It is
    // also the only answer that keeps the attributes below meaning anything - gravity
    // and tileMode are BitmapDrawable's, and a lattice would override both.
    if (!image.ninePatchChunk.empty()) {
        Logger::d(TAG, "<bitmap> android:src is a 9-patch; its stretch regions are "
                       "ignored, as on a real device. Use <nine-patch> to honour them");
    }
    auto drawable = makeBitmapDrawable(image);
    drawable->setGravity(gravity);
    // android:tileMode sets both axes and the per-axis attributes then override it,
    // which is the order AOSP applies them in - so tileMode="repeat" tileModeY="clamp"
    // repeats horizontally and clamps vertically.
    if (tileMode != BD::TILE_MODE_UNDEFINED) {
        const BD::TileMode mode = BD::parseTileMode(tileMode);
        drawable->setTileModeXY(mode, mode);
    }
    if (tileModeX != BD::TILE_MODE_UNDEFINED) {
        drawable->setTileModeX(BD::parseTileMode(tileModeX));
    }
    if (tileModeY != BD::TILE_MODE_UNDEFINED) {
        drawable->setTileModeY(BD::parseTileMode(tileModeY));
    }
    drawable->setAntiAlias(antiAlias);
    drawable->setFilterBitmap(filter);
    if (alpha != 1.0f) {
        drawable->setAlpha(alphaFloatToInt(alpha));
    }
    // android:dither is not mentioned: it means nothing at 32bpp, so silence is
    // honest. These three are different - each would change the pixels.
    if (hasTint) {
        Logger::d(TAG, "<bitmap> android:tint ignored until a ColorFilter exists");
    }
    if (hasMipMap) {
        Logger::d(TAG, "<bitmap> android:mipMap ignored; D2D picks its own sampling");
    }
    if (hasAutoMirrored) {
        Logger::d(TAG, "<bitmap> android:autoMirrored ignored; this runtime is LTR-only");
    }
    return drawable;
}
graphics::DrawablePtr DrawableInflater::inflateNinePatch(android::ResXMLParser* parser,
                                                        ResourceManager* resManager, Theme* theme) {
    // Same ordering constraint as inflateBitmap.
    const XmlAttrs attrs(parser, resManager, theme);
    const uint32_t srcId = attrs.getResourceId("src");
    const float alpha = attrs.getFloat("alpha", 1.0f);
    const bool hasTint = attrs.has("tint");
    const bool hasAutoMirrored = attrs.has("autoMirrored");
    skipCurrentElement(parser);
    if (srcId == 0) {
        Logger::w(TAG, "<nine-patch> without a resolvable android:src; no background drawn");
        return nullptr;
    }
    DecodedImage image;
    if (!decodeImageResource(resManager, theme, srcId, image)) return nullptr;
    // No chunk check before building. AOSP throws "<nine-patch> requires a valid
    // 9-patch source image" here, but this class already degrades to a plain stretch
    // and says so, and that is the more useful outcome for a widget that would
    // otherwise lose its background entirely.
    auto drawable = makeNinePatchDrawable(image, "<nine-patch> android:src");
    if (alpha != 1.0f) {
        drawable->setAlpha(alphaFloatToInt(alpha));
    }
    // android:dither omitted for the same reason as in <bitmap>.
    if (hasTint) {
        Logger::d(TAG, "<nine-patch> android:tint ignored until a ColorFilter exists");
    }
    if (hasAutoMirrored) {
        Logger::d(TAG, "<nine-patch> android:autoMirrored ignored; this runtime is LTR-only");
    }
    return drawable;
}
graphics::DrawablePtr DrawableInflater::inflateShape(android::ResXMLParser* parser,
                                                    ResourceManager* resManager, Theme* theme) {
    using Shape = graphics::GradientDrawable::Shape;
    auto shape = std::make_shared<graphics::GradientDrawable>();
    {
        const XmlAttrs attrs(parser, resManager, theme);
        // The enum ordinals AAPT compiles android:shape to are the same numbers
        // GradientDrawable.Shape uses.
        Shape shapeKind = Shape::RECTANGLE;
        switch (attrs.getInt("shape", (int)Shape::RECTANGLE)) {
            case (int)Shape::OVAL: shapeKind = Shape::OVAL; break;
            case (int)Shape::LINE: shapeKind = Shape::LINE; break;
            case (int)Shape::RING: shapeKind = Shape::RING; break;
            default: shapeKind = Shape::RECTANGLE; break;
        }
        shape->setShape(shapeKind);
        if (shapeKind == Shape::RING) {
            // AOSP reads each absolute dimension first and only consults the
            // matching ratio when that dimension is absent, so an authored
            // innerRadius wins outright instead of being combined with a ratio.
            // -1 is its "no absolute value" sentinel.
            using G = graphics::GradientDrawable;
            const int innerRadius = attrs.getDimensionPixelSize("innerRadius", -1);
            const int thickness = attrs.getDimensionPixelSize("thickness", -1);
            shape->setRingGeometry(
                (float)innerRadius,
                innerRadius == -1 ? attrs.getFloat("innerRadiusRatio",
                                                   G::DEFAULT_INNER_RADIUS_RATIO)
                                  : G::DEFAULT_INNER_RADIUS_RATIO,
                (float)thickness,
                thickness == -1 ? attrs.getFloat("thicknessRatio", G::DEFAULT_THICKNESS_RATIO)
                                : G::DEFAULT_THICKNESS_RATIO);
        }
        // Still unread here: android:tint, and android:useLevel - which would make
        // a ring a partial arc driven by setLevel(). Without it a ring is always the
        // full annulus, which is what an unlevelled one draws anyway.
    }
    // A <gradient> outranks a <solid> in AOSP regardless of which came first, so
    // its stand-in colour is held back and applied once the element is fully read.
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) break;   // </shape>
            --depth;
            continue;
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        // Grandchildren are not a thing inside <shape>; ignore anything nested
        // deeper rather than mistaking it for a direct child.
        if (depth++ > 0) continue;
        const std::string tag = elementName(parser);
        const XmlAttrs attrs(parser, resManager, theme);
        if (tag == "solid") {
            uint32_t color = 0;
            if (readColor(attrs, "color", color)) {
                shape->setColor(color);
            } else if (auto csl = attrs.getColorStateList("color")) {
                // A res/color/*.xml selector: the fill follows the owner's state.
                // This is what a stock Material button background is made of.
                shape->setColor(csl);
            } else {
                Logger::d(TAG, "<solid> has no usable android:color; fill left unset");
            }
        } else if (tag == "corners") {
            // AOSP reads the shared radius first, then each corner defaulting to
            // it, and only builds the per-corner array when one of them differs.
            const int radius = attrs.getDimensionPixelSize("radius", 0);
            shape->setCornerRadius((float)radius);
            const int topLeft = attrs.getDimensionPixelSize("topLeftRadius", radius);
            const int topRight = attrs.getDimensionPixelSize("topRightRadius", radius);
            const int bottomLeft = attrs.getDimensionPixelSize("bottomLeftRadius", radius);
            const int bottomRight = attrs.getDimensionPixelSize("bottomRightRadius", radius);
            if (topLeft != radius || topRight != radius ||
                bottomLeft != radius || bottomRight != radius) {
                const float radii[8] = {
                    (float)topLeft, (float)topLeft,
                    (float)topRight, (float)topRight,
                    (float)bottomRight, (float)bottomRight,
                    (float)bottomLeft, (float)bottomLeft
                };
                shape->setCornerRadii(radii);
            }
        } else if (tag == "stroke") {
            const int width = attrs.getDimensionPixelSize("width", (int)shape->getStrokeWidth());
            // No readable colour means no stroke at all - better than inventing a
            // black outline the real device never draws.
            uint32_t color = 0;
            const bool flat = readColor(attrs, "color", color);
            const float dashWidth = attrs.getDimension("dashWidth", 0.0f);
            const float dashGap = attrs.getDimension("dashGap", 0.0f);
            shape->setStroke((float)width, color, dashWidth, dashGap);
            if (!flat) {
                // A stroke colour can be a selector too - an outlined button whose
                // border greys out when disabled is exactly this. Applied after
                // setStroke because setStroke clears any selector by design.
                if (auto csl = attrs.getColorStateList("color")) {
                    shape->setStrokeColorStateList(csl);
                }
            }
        } else if (tag == "padding") {
            // getDimensionPixelOffset, not PixelSize: padding truncates in AOSP.
            shape->setPaddingInsets(attrs.getDimensionPixelOffset("left", 0),
                                    attrs.getDimensionPixelOffset("top", 0),
                                    attrs.getDimensionPixelOffset("right", 0),
                                    attrs.getDimensionPixelOffset("bottom", 0));
        } else if (tag == "size") {
            shape->setSize(attrs.getDimensionPixelSize("width", -1),
                           attrs.getDimensionPixelSize("height", -1));
        } else if (tag == "gradient") {
            uint32_t start = 0, center = 0, end = 0;
            const bool hasStart = readColor(attrs, "startColor", start);
            const bool hasCenter = readColor(attrs, "centerColor", center);
            const bool hasEnd = readColor(attrs, "endColor", end);
            
            float angle = attrs.getFloat("angle", 0.0f);
            float centerX = attrs.getFraction("centerX", 0.5f);
            float centerY = attrs.getFraction("centerY", 0.5f);
            float gradientRadius = attrs.getDimensionPixelSize("gradientRadius", -1.0f);
            if (gradientRadius == -1.0f) {
                gradientRadius = attrs.getFraction("gradientRadius", -1.0f); // In case it's fraction
            }
            
            int typeInt = attrs.getInt("type", 0);
            graphics::GradientDrawable::GradientType type = graphics::GradientDrawable::GradientType::LINEAR;
            if (typeInt == 1) type = graphics::GradientDrawable::GradientType::RADIAL;
            else if (typeInt == 2) type = graphics::GradientDrawable::GradientType::SWEEP;

            shape->setGradient(type, angle, centerX, centerY, gradientRadius,
                               start, center, end, hasCenter, graphics::TileMode::CLAMP);
        }
        // <shape> has no other children worth reading: everything else AAPT would
        // let through is a no-op on a real device too.
    }
    return shape;
}

graphics::DrawablePtr DrawableInflater::inflateInset(android::ResXMLParser* parser,
                                                     ResourceManager* resManager, Theme* theme) {
    int insetLeft = 0, insetTop = 0, insetRight = 0, insetBottom = 0;
    {
        const XmlAttrs attrs(parser, resManager, theme);
        int inset = attrs.getDimensionPixelOffset("inset", 0);
        insetLeft = attrs.getDimensionPixelOffset("insetLeft", inset);
        insetTop = attrs.getDimensionPixelOffset("insetTop", inset);
        insetRight = attrs.getDimensionPixelOffset("insetRight", inset);
        insetBottom = attrs.getDimensionPixelOffset("insetBottom", inset);
    }
    graphics::DrawablePtr child;
    int depth = 0;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::END_TAG) {
            if (depth == 0) break;
            --depth;
            continue;
        }
        if (event != android::ResXMLParser::START_TAG) continue;
        
        if (depth++ > 0) continue;
        if (!child) {
            child = inflateFromParser(parser, resManager, theme);
        } else {
            Logger::d(TAG, "<inset> has more than one child; ignored");
            skipCurrentElement(parser);
        }
    }
    if (!child) {
        Logger::w(TAG, "<inset> without a drawable child");
        return nullptr;
    }
    return std::make_shared<graphics::InsetDrawable>(std::move(child), insetLeft, insetTop, insetRight, insetBottom);
}
} // namespace setu


