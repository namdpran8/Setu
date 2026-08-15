#include "Theme.h"
#include "../utils/Logger.h"

namespace setu {

Theme::Theme(ResourceManager* resManager) : m_resManager(resManager) {
    if (m_resManager && m_resManager->getAssetManager()) {
        m_theme = m_resManager->getAssetManager()->NewTheme();
    }
}

Theme::~Theme() {
}

void Theme::applyStyle(uint32_t styleResId, bool force) {
    if (!m_theme) return;
    
    if (styleResId == 0) return;

    if (!m_theme->ApplyStyle(styleResId, force).has_value()) {
        Logger::w("Theme", "applyStyle: Failed to apply style ID " + std::to_string(styleResId));
    }
}

} // namespace setu

