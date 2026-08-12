#include "androidfw/Util.h"
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
#include "androidfw/ResourceUtils.h"
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

std::shared_ptr<windroid::view::View> LayoutInflater::inflate(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme) {
    if (!parser) return nullptr;

    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT && event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            return inflateRecursive(parser, nullptr, resManager, theme);
        }
    }
    return nullptr;
}

std::shared_ptr<windroid::view::View> LayoutInflater::inflateRecursive(android::ResXMLParser* parser, std::shared_ptr<windroid::view::ViewGroup> parent, ResourceManager* resManager, Theme* theme) {
    size_t tagLen;
    const char16_t* tag16 = parser->getElementName(&tagLen);
    if (!tag16) return nullptr;
    
    std::string tag = android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen));
    std::shared_ptr<windroid::view::View> view = nullptr;

    if (tag.find("ConstraintLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::ConstraintLayout>();
    } else if (tag.find("GridLayout") != std::string::npos) {
        auto gl = std::make_shared<windroid::view::GridLayout>();
        for (size_t i = 0; i < parser->getAttributeCount(); i++) {
            size_t nameLen;
            const char16_t* name16 = parser->getAttributeName(i, &nameLen);
            std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
            
            if (attrName == "columnCount") {
                gl->setColumnCount(parser->getAttributeData(i));
            } else if (attrName == "rowCount") {
                gl->setRowCount(parser->getAttributeData(i));
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
            ll->setOrientation(windroid::view::LinearLayout::Orientation::HORIZONTAL); // default
            for (size_t i = 0; i < parser->getAttributeCount(); i++) {
                size_t nameLen;
                const char16_t* name16 = parser->getAttributeName(i, &nameLen);
                std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
                
                if (attrName == "orientation") {
                    if (parser->getAttributeData(i) == 1) { // 1 is vertical in Android
                        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
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
        view = std::make_shared<windroid::widget::TextView>(resManager, theme, parser, 0, 0);
    } else if (tag.find("Button") != std::string::npos) {
        view = std::make_shared<windroid::widget::Button>(resManager, theme, parser, 0, 0);
    } else if (tag.find("EditText") != std::string::npos) {
        view = std::make_shared<windroid::widget::EditText>(resManager, theme, parser, 0, 0);
    } else if (tag.find("Guideline") != std::string::npos || tag.find("Space") != std::string::npos || tag == "View" || tag == "android.view.View") {
        view = std::make_shared<windroid::view::View>(resManager, theme, parser, 0, 0);
    } else if (tag.find("HorizontalScrollView") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::HORIZONTAL);
        view = ll;
    } else if (tag.find("RecyclerView") != std::string::npos || tag.find("ScrollView") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else if (tag.find("SlidingUpPanelLayout") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        ll->setOrientation(windroid::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else {
        Logger::w("LayoutInflater", "Unsupported view tag: " + tag + ", falling back to FrameLayout");
        view = std::make_shared<windroid::view::FrameLayout>();
    }

    parseViewAttributes(parser, view, resManager, theme);
    parseLayoutParams(parser, view, parent);

    auto viewGroup = std::dynamic_pointer_cast<windroid::view::ViewGroup>(view);
    
    int depth = 1;
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT && event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            auto childView = inflateRecursive(parser, viewGroup, resManager, theme);
            if (childView && viewGroup) {
                viewGroup->addView(childView);
            }
        } else if (event == android::ResXMLParser::END_TAG) {
            depth--;
            if (depth == 0) break;
        }
    }

    return view;
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

void LayoutInflater::parseViewAttributes(android::ResXMLParser* parser, std::shared_ptr<windroid::view::View> view, ResourceManager* resManager, Theme* theme) {
    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        
        if (attrName == "id") {
            // Check if it's a reference (e.g. @+id/...)
            int type = parser->getAttributeDataType(i);
            if (type == android::Res_value::TYPE_REFERENCE) {
                view->setId(parser->getAttributeData(i));
            }
        } else if (attrName == "visibility") {
            int val = parser->getAttributeData(i);
            if (val == 0) view->setVisibility(windroid::view::View::VISIBLE);
            else if (val == 1) view->setVisibility(windroid::view::View::INVISIBLE);
            else if (val == 2) view->setVisibility(windroid::view::View::GONE);
        }
    }
}

void LayoutInflater::parseLayoutParams(android::ResXMLParser* parser, std::shared_ptr<windroid::view::View> view, std::shared_ptr<windroid::view::ViewGroup> parent) {
    std::shared_ptr<windroid::view::View::LayoutParams> lp;
    if (parent) {
        lp = parent->generateLayoutParams(parser);
    } else {
        lp = std::make_shared<windroid::view::View::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    }

    auto parseDim = [&](size_t idx) {
        int type = parser->getAttributeDataType(idx);
        uint32_t data = parser->getAttributeData(idx);
        if (type == android::Res_value::TYPE_DIMENSION) {
            return LayoutInflater::parseComplexDimension(data);
        }
        return (int)data;
    };

    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        
        if (attrName == "layout_width") {
            int type = parser->getAttributeDataType(i);
            uint32_t data = parser->getAttributeData(i);
            if (type == android::Res_value::TYPE_INT_DEC) {
                if (data == 0xFFFFFFFF) lp->width = windroid::view::View::MATCH_PARENT;
                else if (data == 0xFFFFFFFE) lp->width = windroid::view::View::WRAP_CONTENT;
                else lp->width = (int)data;
            } else {
                lp->width = parseDim(i);
            }
        } else if (attrName == "layout_height") {
            int type = parser->getAttributeDataType(i);
            uint32_t data = parser->getAttributeData(i);
            if (type == android::Res_value::TYPE_INT_DEC) {
                if (data == 0xFFFFFFFF) lp->height = windroid::view::View::MATCH_PARENT;
                else if (data == 0xFFFFFFFE) lp->height = windroid::view::View::WRAP_CONTENT;
                else lp->height = (int)data;
            } else {
                lp->height = parseDim(i);
            }
        } else if (attrName == "layout_marginLeft" || attrName == "layout_marginStart") {
            lp->leftMargin = parseDim(i);
        } else if (attrName == "layout_marginTop") {
            lp->topMargin = parseDim(i);
        } else if (attrName == "layout_marginRight" || attrName == "layout_marginEnd") {
            lp->rightMargin = parseDim(i);
        } else if (attrName == "layout_marginBottom") {
            lp->bottomMargin = parseDim(i);
        }
    }

    view->setLayoutParams(lp);
}

} // namespace windroid



