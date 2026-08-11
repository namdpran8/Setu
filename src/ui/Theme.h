#pragma once
#include <cstdint>
#include <unordered_map>
#include "../utils/AndroidRes.h"
#include "../dex/ResourceManager.h"

namespace windroid {

class Theme {
public:
    Theme(ResourceManager* resManager);
    ~Theme();

    // Applies a style bag to this theme.
    // If force is true, it overrides existing attributes. If false, existing attributes are kept.
    void applyStyle(uint32_t styleResId, bool force);

    // Resolves an attribute value, handling ?attr/ references recursively if needed.
    // Returns true if the attribute was found in the theme.
    bool resolveAttribute(uint32_t attrResId, Res_value& outValue) const;

    // Direct access to the flattened map
    const std::unordered_map<uint32_t, Res_value>& getFlattenedAttributes() const { return m_attributes; }

private:
    ResourceManager* m_resManager;
    
    // The flattened theme attributes. Map of attribute Resource ID -> Res_value
    std::unordered_map<uint32_t, Res_value> m_attributes;
};

} // namespace windroid
