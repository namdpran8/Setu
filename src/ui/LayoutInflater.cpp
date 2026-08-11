#include "LayoutInflater.h"
#include "../utils/Logger.h"
#include "../widget/Button.h"
#include "../widget/TextView.h"
#include "../widget/EditText.h"
#include "../view/LinearLayout.h"
#include "../view/FrameLayout.h"
#include "../view/RelativeLayout.h"
#include "../view/ConstraintLayout.h"
#include "../view/GridLayout.h"
#include <string>
#include <cwchar>

namespace windroid {

// Helper to convert std::string to std::wstring
static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::shared_ptr<windroid::view::View> LayoutInflater::inflate(const AxmlNode* node, ResourceManager* resManager, Theme* theme) {
    if (!node) return nullptr;
    return inflateRecursive(node, resManager, theme);
}

std::shared_ptr<windroid::view::View> LayoutInflater::inflateRecursive(const AxmlNode* node, ResourceManager* resManager, Theme* theme) {
    std::string tag = node->tag;
    std::shared_ptr<windroid::view::View> view = nullptr;

    if (tag.find("ConstraintLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::ConstraintLayout>();
    } else if (tag.find("GridLayout") != std::string::npos) {
        auto gl = std::make_shared<windroid::view::GridLayout>();
        for (const auto& attr : node->attributes) {
            if (attr.name == "columnCount") {
                gl->setColumnCount(attr.typedValueData);
            } else if (attr.name == "rowCount") {
                gl->setRowCount(attr.typedValueData);
            }
        }
        view = gl;
    } else if (tag.find("LinearLayout") != std::string::npos || tag.find("TableLayout") != std::string::npos || tag.find("TableRow") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        
        // Check orientation
        if (tag.find("TableLayout") != std::string::npos) {
            ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
        } else if (tag.find("TableRow") != std::string::npos) {
            ll->setOrientation(windroid::view::LinearLayout::Orientation::HORIZONTAL);
        } else {
            for (const auto& attr : node->attributes) {
                if (attr.name == "orientation") {
                    if (attr.typedValueData == 1 || resolveString(attr, resManager) == "vertical") {
                        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
                    } else {
                        ll->setOrientation(windroid::view::LinearLayout::Orientation::HORIZONTAL);
                    }
                    break;
                }
            }
        }
        view = ll;
    } else if (tag.find("FrameLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::FrameLayout>();
    } else if (tag.find("RelativeLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::RelativeLayout>();
    } else if (tag.find("TextView") != std::string::npos) {
        view = std::make_shared<windroid::widget::TextView>(resManager, theme, node, 0, 0);
    } else if (tag.find("Button") != std::string::npos) {
        view = std::make_shared<windroid::widget::Button>(resManager, theme, node, 0, 0);
    } else if (tag.find("EditText") != std::string::npos) {
        view = std::make_shared<windroid::widget::EditText>(resManager, theme, node, 0, 0);
    } else if (tag.find("Guideline") != std::string::npos || tag.find("Space") != std::string::npos || tag == "View" || tag == "android.view.View") {
        view = std::make_shared<windroid::view::View>(resManager, theme, node, 0, 0);
    } else if (tag.find("HorizontalScrollView") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::HORIZONTAL);
        view = ll;
    } else if (tag.find("RecyclerView") != std::string::npos || tag.find("ScrollView") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else if (tag.find("SlidingUpPanelLayout") != std::string::npos) {
        // Fallback: Use Vertical LinearLayout so the main content takes the screen
        // and the sliding panel is pushed down off-screen (since main content is match_parent).
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else if (tag.find("ConstraintLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::ConstraintLayout>();
    } else {
        Logger::w("LayoutInflater", "Unsupported view tag: " + tag + ", falling back to FrameLayout");
        view = std::make_shared<windroid::view::FrameLayout>();
    }

    parseViewAttributes(node, view, resManager, theme);

    // Parse children if it is a ViewGroup
    auto viewGroup = std::dynamic_pointer_cast<windroid::view::ViewGroup>(view);
    if (viewGroup) {
        for (const auto& childNode : node->children) {
            auto childView = inflateRecursive(childNode.get(), resManager, theme);
            if (childView) {
                parseLayoutParams(childNode.get(), childView, viewGroup);
                viewGroup->addView(childView);
            }
        }
    }

    return view;
}

std::string LayoutInflater::resolveString(const AxmlAttribute& attr, ResourceManager* resManager) {
    if (attr.typedValueType == 0x03) { // TYPE_STRING
        if (!attr.rawValue.empty()) {
            return attr.rawValue;
        }
    } else if (attr.typedValueType == 0x01) { // TYPE_REFERENCE
        if (resManager) {
            return resManager->getString(attr.typedValueData);
        }
    }
    return std::to_string(attr.typedValueData);
}

int LayoutInflater::parseDimension(const std::string& dimenStr) {
    if (dimenStr.empty()) return 0;
    try {
        if (dimenStr.find("dip") != std::string::npos || dimenStr.find("dp") != std::string::npos) {
            return std::stoi(dimenStr) * 2; // naive density multiplier
        } else if (dimenStr.find("sp") != std::string::npos) {
            return std::stoi(dimenStr) * 2;
        } else if (dimenStr.find("px") != std::string::npos) {
            return std::stoi(dimenStr);
        }
        return std::stoi(dimenStr);
    } catch (...) {
        return 0;
    }
}

int LayoutInflater::parseComplexDimension(uint32_t data) {
    int value = (int)(data >> 8);
    int unit = data & 0x0F;
    if (unit == 1 || unit == 2) { // dp or sp
        return value * 2;
    }
    return value; // px or others
}

void LayoutInflater::parseViewAttributes(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, ResourceManager* resManager, Theme* theme) {
    for (const auto& attr : node->attributes) {
        if (attr.name == "id") {
            view->setId(attr.typedValueData);
        } else if (attr.name == "visibility") {
            if (attr.typedValueData == 0) view->setVisibility(windroid::view::View::VISIBLE);
            else if (attr.typedValueData == 1) view->setVisibility(windroid::view::View::INVISIBLE);
            else if (attr.typedValueData == 2) view->setVisibility(windroid::view::View::GONE);
        }
    }
}

void LayoutInflater::parseLayoutParams(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, std::shared_ptr<windroid::view::ViewGroup> parent) {
    std::shared_ptr<windroid::view::View::LayoutParams> lp;
    if (parent) {
        lp = parent->generateLayoutParams(node);
    } else {
        lp = std::make_shared<windroid::view::View::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    }

    auto parseDim = [](const AxmlAttribute& attr) {
        if (attr.typedValueType == 0x05) return LayoutInflater::parseComplexDimension(attr.typedValueData);
        return (int)attr.typedValueData;
    };

    for (const auto& attr : node->attributes) {
        if (attr.name == "layout_width") {
            if (attr.typedValueData == 0xFFFFFFFF) lp->width = windroid::view::View::MATCH_PARENT;
            else if (attr.typedValueData == 0xFFFFFFFE) lp->width = windroid::view::View::WRAP_CONTENT;
            else lp->width = parseDim(attr);
        } else if (attr.name == "layout_height") {
            if (attr.typedValueData == 0xFFFFFFFF) lp->height = windroid::view::View::MATCH_PARENT;
            else if (attr.typedValueData == 0xFFFFFFFE) lp->height = windroid::view::View::WRAP_CONTENT;
            else lp->height = parseDim(attr);
        } else if (attr.name == "layout_marginLeft" || attr.name == "layout_marginStart") {
            lp->leftMargin = parseDim(attr);
        } else if (attr.name == "layout_marginTop") {
            lp->topMargin = parseDim(attr);
        } else if (attr.name == "layout_marginRight" || attr.name == "layout_marginEnd") {
            lp->rightMargin = parseDim(attr);
        } else if (attr.name == "layout_marginBottom") {
            lp->bottomMargin = parseDim(attr);
        }
    }

    view->setLayoutParams(lp);
}

} // namespace windroid
