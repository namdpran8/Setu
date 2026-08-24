#include "ColorStateListInflater.h"

#include <memory>
#include <string>
#include <vector>

#include "StateSetInflater.h"
#include "Theme.h"
#include "XmlAttrs.h"
#include "../dex/ResourceManager.h"
#include "../graphics/drawable/StateSet.h"
#include "../utils/Logger.h"

namespace setu {

namespace {

const char* const TAG = "ColorStateListInflater";

bool endsWith(const std::string& str, const char* suffix) {
    const std::string s(suffix);
    return str.size() >= s.size() && str.compare(str.size() - s.size(), s.size(), s) == 0;
}

// ColorStateList.modulateColor, minus the CAM16 half.
//
// android:alpha scales the authored colour's *own* alpha rather than replacing
// it, so alpha="0.5" on #80ff0000 gives 0x40 and not 0x80. Applied here at
// inflate time, exactly as AOSP does: the resulting list holds already-modulated
// colours, so nothing downstream has to know the attribute existed.
//
// android:lStar (API 31) would replace the colour's perceptual luminance while
// preserving hue and chroma. That needs a CAM16 conversion Windroid has no
// equivalent of, so it is logged and skipped - which leaves the authored colour
// in place. Wrong luminance is a far smaller error than a dropped item.
uint32_t modulateColor(uint32_t baseColor, float alphaMod, float lStar) {
    const bool validLStar = lStar >= 0.0f && lStar <= 100.0f;
    if (validLStar) {
        Logger::d(TAG, "<item> android:lStar=" + std::to_string(lStar) +
                           " ignored; no CAM16 conversion, authored colour kept");
    }
    if (alphaMod == 1.0f) return baseColor;

    const int baseAlpha = (int)((baseColor >> 24) & 0xFF);
    int alpha = (int)((float)baseAlpha * alphaMod + 0.5f);
    if (alpha < 0) alpha = 0;
    if (alpha > 255) alpha = 255;

    return (baseColor & 0x00FFFFFF) | ((uint32_t)alpha << 24);
}

// One <item> of a colour <selector>. Consumes the element.
//
// Colour items are leaves - all their information is in attributes - so unlike a
// drawable item there is never a nested element to descend into.
void inflateItem(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme,
                 std::vector<std::vector<int>>& specs, std::vector<uint32_t>& colors) {
    // Attributes first: XmlAttrs reads through the parser live, so nothing may
    // advance it until both the states and the colour are out.
    const XmlAttrs attrs(parser, resManager, theme);
    std::vector<int> states = extractStateSet(parser, attrs);

    uint32_t color = graphics::ColorStateList::MISSING_ITEM_COLOR;
    if (!readColor(attrs, "color", color)) {
        // AOSP defaults this item to Color.MAGENTA rather than dropping it, and
        // keeping the item matters: dropping it would let a later item match a
        // state this one was written to claim. Magenta is loud on purpose.
        Logger::w(TAG, "<item> " + graphics::StateSet::describe(states) +
                           " has no readable android:color; using magenta");
    }

    const float alphaMod = attrs.getFloat("alpha", 1.0f);
    const float lStar = attrs.getFloat("lStar", -1.0f);

    specs.push_back(std::move(states));
    colors.push_back(modulateColor(color, alphaMod, lStar));

    skipCurrentElement(parser);
}

} // namespace

graphics::ColorStateListPtr ColorStateListInflater::inflate(ResourceManager* resManager,
                                                           Theme* theme, uint32_t resId) {
    if (!resManager || resId == 0) return nullptr;

    const std::string path = resManager->getResourceFilePath(resId);

    if (path.empty()) {
        // Not a file, so most often @color/foo declared as a literal in
        // values/colors.xml. A constant list is the right answer, not a failure:
        // it is what makes every consumer able to hold a ColorStateList and stop
        // branching on how the author spelled the colour.
        auto* assets = resManager->getAssetManager();
        if (!assets) return nullptr;

        auto res = assets->GetResource(resId);
        if (!res.has_value()) return nullptr;

        android::AssetManager2::SelectedValue val = res.value();
        if (!resManager->resolveValue(val, theme)) return nullptr;

        if (val.type >= android::Res_value::TYPE_FIRST_INT &&
            val.type <= android::Res_value::TYPE_LAST_INT) {
            return graphics::ColorStateList::valueOf(val.data);
        }
        Logger::d(TAG, "Resource 0x" + std::to_string(resId) +
                           " is neither a file nor a colour. Type=" + std::to_string(val.type));
        return nullptr;
    }

    if (!endsWith(path, ".xml")) {
        Logger::d(TAG, "Not a colour resource: " + path);
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
    Logger::w(TAG, "No root element in colour XML: " + path);
    return nullptr;
}

graphics::ColorStateListPtr ColorStateListInflater::inflateFromParser(
        android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme) {
    if (!parser) return nullptr;

    const std::string tag = elementName(parser);
    if (tag != "selector") {
        // res/color holds nothing else. A <shape> here means the caller passed a
        // drawable ID to the colour inflater, which is a caller bug worth naming.
        Logger::d(TAG, "Root <" + tag + "> is not a colour <selector>; not a ColorStateList");
        return nullptr;
    }

    std::vector<std::vector<int>> specs;
    std::vector<uint32_t> colors;

    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        // Every branch consumes one whole element, so the parser is always back at
        // this level next turn - which makes any END_TAG seen here </selector>.
        if (event == android::ResXMLParser::END_TAG) break;
        if (event != android::ResXMLParser::START_TAG) continue;

        const std::string child = elementName(parser);
        if (child == "item") {
            inflateItem(parser, resManager, theme, specs, colors);
        } else {
            Logger::d(TAG, "<selector> child <" + child + "> is not an item; ignored");
            skipCurrentElement(parser);
        }
    }

    if (specs.empty()) {
        // An empty list would report DEFAULT_COLOR red for every state. Reporting
        // "not a colour list" instead lets the caller keep its own default, which
        // is nearly always the better outcome.
        Logger::w(TAG, "Colour <selector> had no usable items");
        return nullptr;
    }

    auto csl = std::make_shared<graphics::ColorStateList>(std::move(specs), std::move(colors));
    Logger::d(TAG, "Inflated " + csl->describe());
    return csl;
}

} // namespace setu
