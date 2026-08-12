#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include "androidfw/ResourceTypes.h"
#include "../dex/ResourceManager.h"
#include "Theme.h"
#include "../view/View.h"

namespace windroid {
namespace view { class View; class ViewGroup; }

class LayoutInflater {
public:
    static std::shared_ptr<windroid::view::View> inflate(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme = nullptr);
    
private:
    static std::shared_ptr<windroid::view::View> inflateRecursive(android::ResXMLParser* parser, std::shared_ptr<windroid::view::ViewGroup> parent, ResourceManager* resManager, Theme* theme);
    
    // Helpers to parse common attributes
    static void parseViewAttributes(android::ResXMLParser* parser, std::shared_ptr<windroid::view::View> view, ResourceManager* resManager, Theme* theme);
    static void parseLayoutParams(android::ResXMLParser* parser, std::shared_ptr<windroid::view::View> view, std::shared_ptr<windroid::view::ViewGroup> parent);
    static int parseDimension(const std::string& dimenStr);
    static int parseComplexDimension(uint32_t data);
};

} // namespace windroid
