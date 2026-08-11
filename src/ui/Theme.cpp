#include "Theme.h"
#include "../utils/Logger.h"

namespace windroid {

Theme::Theme(ResourceManager* resManager) : m_resManager(resManager) {
}

Theme::~Theme() {
}

void Theme::applyStyle(uint32_t styleResId, bool force) {
    if (!m_resManager) return;
    
    if (styleResId == 0) return;

    const ArscParser::Bag* bag = m_resManager->getBag(styleResId);
    if (!bag) {
        Logger::w("Theme", "applyStyle: Could not find style bag for ID " + std::to_string(styleResId));
        return;
    }

    // AOSP applies parent styles first, so the child's overrides take precedence
    if (bag->parentResId != 0 && bag->parentResId != 0xFFFFFFFF) {
        applyStyle(bag->parentResId, force);
    }

    // Now apply the current bag's attributes
    for (const auto& map : bag->maps) {
        if (!force) {
            // If not forcing, we keep existing attributes
            if (m_attributes.find(map.name) != m_attributes.end()) {
                continue;
            }
        }
        m_attributes[map.name] = map.value;
    }
}

bool Theme::resolveAttribute(uint32_t attrResId, Res_value& outValue) const {
    int maxDepth = 10;
    uint32_t currentAttr = attrResId;

    while (maxDepth-- > 0) {
        auto it = m_attributes.find(currentAttr);
        if (it == m_attributes.end()) {
            return false;
        }

        const Res_value& val = it->second;
        
        // 0x02 is TYPE_ATTRIBUTE (?attr/reference)
        if (val.dataType == 0x02) {
            currentAttr = val.data;
            continue; // Loop again to resolve the referenced attribute
        }

        outValue = val;
        return true;
    }

    Logger::e("Theme", "resolveAttribute: Max depth exceeded for attribute " + std::to_string(attrResId));
    return false;
}

} // namespace windroid
