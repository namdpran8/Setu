#include "AnimatorInflater.h"
#include "ObjectAnimator.h"
#include "AnimatorSet.h"
#include "../ui/XmlAttrs.h"
#include "../dex/ResourceManager.h"
#include "../utils/Logger.h"

#include <androidfw/ResourceTypes.h>

namespace setu {
namespace animation {

static const std::string TAG = "AnimatorInflater";

std::string getElementName(android::ResXMLParser* parser) {
    size_t len;
    const char16_t* name16 = parser->getElementName(&len);
    if (!name16) return "";
    android::String8 name8(name16, len);
    return std::string(name8.string());
}

static void skipCurrentElement(android::ResXMLParser* parser) {
    int depth = 1;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            depth++;
        } else if (event == android::ResXMLParser::END_TAG) {
            depth--;
            if (depth == 0) return;
        }
    }
}

std::shared_ptr<ValueAnimator> AnimatorInflater::loadAnimator(ResourceManager* resManager, Theme* theme, uint32_t resId) {
    if (!resManager || resId == 0) return nullptr;

    const std::string path = resManager->getResourceFilePath(resId);
    if (path.empty()) return nullptr;

    auto tree = resManager->openXml(resId);
    if (!tree) {
        Logger::e(TAG, "Failed to load XML for animator " + path);
        return nullptr;
    }

    android::ResXMLParser::event_code_t event;
    while ((event = tree->next()) != android::ResXMLParser::BAD_DOCUMENT &&
           event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            return inflateAnimator(tree.get(), resManager, theme);
        }
    }
    
    return nullptr;
}

std::shared_ptr<ValueAnimator> AnimatorInflater::inflateAnimator(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme) {
    std::string tag = getElementName(parser);
    XmlAttrs attrs(parser, resManager, theme);

    if (tag == "objectAnimator") {
        std::string propertyName = attrs.getString("propertyName");
        long long duration = attrs.getInt("duration", 300);
        float valueFrom = attrs.getFloat("valueFrom", 0.0f);
        float valueTo = attrs.getFloat("valueTo", 1.0f);
        
        // Known gap: We are currently ignoring android:interpolator
        if (attrs.has("interpolator")) {
            Logger::d(TAG, "Ignoring android:interpolator in objectAnimator for now");
        }

        auto anim = ObjectAnimator::ofFloat(nullptr, "", propertyName, {valueFrom, valueTo});
        anim->setDuration(duration);
        
        skipCurrentElement(parser);
        return anim;
    } else if (tag == "set") {
        auto set = std::make_shared<AnimatorSet>();
        std::vector<std::shared_ptr<ValueAnimator>> children;
        
        android::ResXMLParser::event_code_t event;
        int depth = 1;
        while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT &&
               event != android::ResXMLParser::END_DOCUMENT) {
            if (event == android::ResXMLParser::END_TAG) {
                depth--;
                if (depth == 0) break;
            } else if (event == android::ResXMLParser::START_TAG) {
                if (depth == 1) {
                    auto childAnim = inflateAnimator(parser, resManager, theme);
                    if (childAnim) {
                        children.push_back(childAnim);
                    }
                } else {
                    skipCurrentElement(parser);
                }
            }
        }
        
        // For now, always play together
        set->playTogether(children);
        return set;
    }

    Logger::w(TAG, "Unknown animator tag: " + tag);
    skipCurrentElement(parser);
    return nullptr;
}

}
}
