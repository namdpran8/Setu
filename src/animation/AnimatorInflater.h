#pragma once

#include <memory>
#include <string>
#include "ValueAnimator.h"

namespace android {
class ResXMLParser;
}

namespace setu {
class ResourceManager;
class Theme;

namespace animation {

class AnimatorInflater {
public:
    // Inflates an animator from the given resource ID
    static std::shared_ptr<ValueAnimator> loadAnimator(ResourceManager* resManager, Theme* theme, uint32_t resId);

private:
    static std::shared_ptr<ValueAnimator> inflateAnimator(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme);
};

} // namespace animation
} // namespace setu
