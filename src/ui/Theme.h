#pragma once
#include <cstdint>
#include <memory>
#include "androidfw/AssetManager2.h"
#include "../dex/ResourceManager.h"

namespace setu {

class Theme {
public:
    Theme(ResourceManager* resManager);
    ~Theme();

    // Applies a style bag to this theme.
    // If force is true, it overrides existing attributes. If false, existing attributes are kept.
    void applyStyle(uint32_t styleResId, bool force);

    android::Theme* getTheme() const { return m_theme.get(); }

private:
    ResourceManager* m_resManager;
    std::unique_ptr<android::Theme> m_theme;
};

} // namespace setu
