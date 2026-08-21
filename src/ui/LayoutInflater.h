#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include <string>
#include "androidfw/ResourceTypes.h"
#include "../dex/ResourceManager.h"
#include "Theme.h"
#include "../view/View.h"

namespace setu {
namespace view { class View; class ViewGroup; }

class LayoutInflater {
public:
    static std::shared_ptr<setu::view::View> inflate(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme = nullptr);
    static int parseDimension(const std::string& dimenStr);
    static int parseComplexDimension(uint32_t data);
private:
    static std::shared_ptr<setu::view::View> inflateRecursive(android::ResXMLParser* parser, std::shared_ptr<setu::view::ViewGroup> parent, ResourceManager* resManager, Theme* theme);
    
    // Helpers to parse common attributes
    static void parseViewAttributes(android::ResXMLParser* parser, std::shared_ptr<setu::view::View> view, ResourceManager* resManager, Theme* theme);
    static void parseLayoutParams(android::ResXMLParser* parser, std::shared_ptr<setu::view::View> view, std::shared_ptr<setu::view::ViewGroup> parent, ResourceManager* resManager, Theme* theme);
};

} // namespace setu
