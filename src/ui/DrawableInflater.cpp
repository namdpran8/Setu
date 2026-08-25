#include "DrawableInflater.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "StateSetInflater.h"
#include "Theme.h"
#include "XmlAttrs.h"
#include "../dex/ResourceManager.h"
#include "../graphics/drawable/ColorDrawable.h"
#include "../graphics/drawable/GradientDrawable.h"
#include "../graphics/drawable/RippleDrawable.h"
#include "../graphics/drawable/StateListDrawable.h"
#include "../graphics/drawable/StateSet.h"
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
    if (tag == "bitmap" || tag == "nine-patch") return "Phase 6 (bitmap pipeline)";
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
        // .png, .9.png, .webp, .jpg - all of them need a decoder and a
        // Canvas::drawBitmap, which is Phase 6.
        Logger::d(TAG, "Skipping bitmap drawable (Phase 6): " + path);
        return nullptr;
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
    bool hasGradientColor = false;
    uint32_t gradientColor = 0;

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
            // Real gradients need a shader on Paint, which is a later roadmap
            // item. Averaging the stops into one flat colour is wrong in a
            // pixel diff, but a gradient-only <shape> has no <solid> to fall back
            // on, so the alternative is a view that vanishes completely. Delete
            // this branch when the shader lands.
            uint32_t start = 0, center = 0, end = 0;
            const bool hasStart = readColor(attrs, "startColor", start);
            const bool hasCenter = readColor(attrs, "centerColor", center);
            const bool hasEnd = readColor(attrs, "endColor", end);

            if (hasStart && hasEnd) {
                gradientColor = hasCenter ? meanColor3(start, center, end) : meanColor(start, end);
                hasGradientColor = true;
            } else if (hasStart) {
                gradientColor = start;
                hasGradientColor = true;
            }
            if (hasGradientColor) {
                Logger::d(TAG, "<gradient> approximated with a flat colour until shaders exist");
            }
        }
        // <shape> has no other children worth reading: everything else AAPT would
        // let through is a no-op on a real device too.
    }

    if (hasGradientColor) {
        shape->setColor(gradientColor);
    }
    return shape;
}

} // namespace setu
