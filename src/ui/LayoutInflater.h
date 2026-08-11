#pragma once
#include <windows.h>
#include <vector>
#include <memory>
#include "../AxmlPraserer/AxmlParser.h"
#include "../dex/ResourceManager.h"
#include "../view/View.h"

namespace windroid {

class LayoutInflater {
public:
    static std::shared_ptr<windroid::view::View> inflate(const AxmlNode* node, ResourceManager* resManager);
    
private:
    static std::shared_ptr<windroid::view::View> inflateRecursive(const AxmlNode* node, ResourceManager* resManager);
    static std::string resolveString(const AxmlAttribute& attr, ResourceManager* resManager);
    
    // Helpers to parse common attributes
    static void parseViewAttributes(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, ResourceManager* resManager);
    static void parseLayoutParams(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, std::shared_ptr<windroid::view::ViewGroup> parent);
    static int parseDimension(const std::string& dimenStr);
    static int parseComplexDimension(uint32_t data);
};

} // namespace windroid
