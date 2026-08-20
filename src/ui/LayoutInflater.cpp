#include "androidfw/Util.h"
#include "LayoutInflater.h"
#include "../utils/Logger.h"
#include "../widget/Button.h"
#include "../widget/ImageView.h"
#include "../widget/ImageButton.h"
#include "../widget/TextView.h"
#include "../widget/EditText.h"
#include "../view/LinearLayout.h"
#include "../view/FrameLayout.h"
#include "../view/OverlayPanelLayout.h"
#include "../view/RelativeLayout.h"
#include "../view/ConstraintLayout.h"
#include "../view/GridLayout.h"
#include "androidfw/ResourceUtils.h"
#include "WindowManager.h"
#include <string>
#include <cwchar>

namespace setu {

// Helper to convert std::string to std::wstring
static std::wstring utf8_to_utf16(const std::string& utf8) {
    if (utf8.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::shared_ptr<setu::view::View> LayoutInflater::inflate(android::ResXMLParser* parser, ResourceManager* resManager, Theme* theme) {
    if (!parser) return nullptr;

    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT && event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            return inflateRecursive(parser, nullptr, resManager, theme);
        }
    }
    return nullptr;
}

std::shared_ptr<setu::view::View> LayoutInflater::inflateRecursive(android::ResXMLParser* parser, std::shared_ptr<setu::view::ViewGroup> parent, ResourceManager* resManager, Theme* theme) {
    size_t tagLen;
    const char16_t* tag16 = parser->getElementName(&tagLen);
    if (!tag16) return nullptr;
    
    std::string tag = android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen));
    std::shared_ptr<setu::view::View> view = nullptr;

    if (tag == "ConstraintLayout" || tag == "androidx.constraintlayout.widget.ConstraintLayout") {
        view = std::make_shared<setu::view::ConstraintLayout>();
    } else if (tag == "GridLayout" || tag == "android.widget.GridLayout" || tag == "androidx.gridlayout.widget.GridLayout") {
        auto gl = std::make_shared<setu::view::GridLayout>();
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
    } else if (tag == "LinearLayout" || tag == "android.widget.LinearLayout" || tag == "TableLayout" ||
               tag == "android.widget.TableLayout" || tag == "TableRow" || tag == "android.widget.TableRow") {
        auto ll = std::make_shared<setu::view::LinearLayout>();
        
        // Check orientation
        if (tag.find("TableLayout") != std::string::npos) {
            ll->setOrientation(setu::view::LinearLayout::Orientation::VERTICAL);
        } else if (tag.find("TableRow") != std::string::npos) {
            ll->setOrientation(setu::view::LinearLayout::Orientation::HORIZONTAL);
        } else {
            ll->setOrientation(setu::view::LinearLayout::Orientation::HORIZONTAL); // default
            for (size_t i = 0; i < parser->getAttributeCount(); i++) {
                size_t nameLen;
                const char16_t* name16 = parser->getAttributeName(i, &nameLen);
                std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
                
                if (attrName == "orientation") {
                    if (parser->getAttributeData(i) == 1) { // 1 is vertical in Android
                        ll->setOrientation(setu::view::LinearLayout::Orientation::VERTICAL);
                    }
                    break;
                }
            }
        }
        view = ll;
    } else if (tag == "FrameLayout" || tag == "android.widget.FrameLayout") {
        view = std::make_shared<setu::view::FrameLayout>();
    } else if (tag == "RelativeLayout" || tag == "android.widget.RelativeLayout") {
        view = std::make_shared<setu::view::RelativeLayout>();
    } else if (tag == "TextView" || tag == "android.widget.TextView" ||
               tag == "androidx.appcompat.widget.AppCompatTextView") {
        view = std::make_shared<setu::widget::TextView>(resManager, theme, parser, 0x01010084, 0);
    } else if (tag == "Button" || tag == "android.widget.Button" ||
               tag == "androidx.appcompat.widget.AppCompatButton") {
        view = std::make_shared<setu::widget::Button>(resManager, theme, parser, 0x01010048, 0);
    } else if (tag == "ImageView" || tag == "android.widget.ImageView" || tag == "androidx.appcompat.widget.AppCompatImageView") {
        view = std::make_shared<setu::widget::ImageView>(resManager, theme, parser, 0x01010064, 0);
    } else if (tag == "ImageButton" || tag == "android.widget.ImageButton" || tag == "androidx.appcompat.widget.AppCompatImageButton") {
        view = std::make_shared<setu::widget::ImageButton>(resManager, theme, parser, 0x01010039, 0);
    } else if (tag == "EditText" || tag == "android.widget.EditText" ||
               tag == "androidx.appcompat.widget.AppCompatEditText" ||
               tag == "AutoCompleteTextView" || tag == "android.widget.AutoCompleteTextView" ||
               tag == "androidx.appcompat.widget.AppCompatAutoCompleteTextView") {
        view = std::make_shared<setu::widget::EditText>(resManager, theme, parser, 0x01010006, 0);
    } else if (tag == "RadioGroup" || tag == "android.widget.RadioGroup") {
        // RadioGroup is a vertical LinearLayout
        auto ll = std::make_shared<setu::view::LinearLayout>();
        ll->setOrientation(setu::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else if (tag == "RadioButton" || tag == "android.widget.RadioButton" ||
               tag == "CheckBox" || tag == "android.widget.CheckBox") {
        view = std::make_shared<setu::widget::Button>(resManager, theme, parser, 0, 0);
    } else if (tag == "com.google.android.material.textfield.TextInputLayout") {
        // TextInputLayout wraps an EditText — treat as a vertical FrameLayout container
        auto fl = std::make_shared<setu::view::FrameLayout>();
        view = fl;
    } else if (tag == "com.google.android.material.textfield.TextInputEditText") {
        view = std::make_shared<setu::widget::EditText>(resManager, theme, parser, 0, 0);
    } else if (tag == "Guideline" || tag == "androidx.constraintlayout.widget.Guideline" ||
               tag == "Space" || tag == "android.widget.Space" || tag == "View" || tag == "android.view.View") {
        view = std::make_shared<setu::view::View>(resManager, theme, parser, 0, 0);
    } else if (tag == "HorizontalScrollView" || tag == "android.widget.HorizontalScrollView") {
        auto ll = std::make_shared<setu::view::LinearLayout>();
        ll->setOrientation(setu::view::LinearLayout::Orientation::HORIZONTAL);
        view = ll;
    } else if (tag == "ScrollView" || tag == "android.widget.ScrollView" || tag == "androidx.recyclerview.widget.RecyclerView") {
        auto ll = std::make_shared<setu::view::LinearLayout>();
        ll->setOrientation(setu::view::LinearLayout::Orientation::VERTICAL);
        view = ll;
    } else if (tag == "com.sothree.slidinguppanel.SlidingUpPanelLayout" || tag == "SlidingUpPanelLayout") {
        view = std::make_shared<setu::view::OverlayPanelLayout>();
    } else {
        Logger::w("LayoutInflater", "Unsupported view tag: " + tag + ", using FrameLayout fallback");
        view = std::make_shared<setu::view::FrameLayout>();
    }

    parseViewAttributes(parser, view, resManager, theme);
    parseLayoutParams(parser, view, parent);

    auto viewGroup = std::dynamic_pointer_cast<setu::view::ViewGroup>(view);
    
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
            return (int)(std::stof(dimenStr) * WindowManager::getDensity());
        } else if (dimenStr.find("sp") != std::string::npos) {
            return (int)(std::stof(dimenStr) * WindowManager::getScaledDensity());
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
    if (unit == 1) { // dp
        return (int)(value * WindowManager::getDensity());
    } else if (unit == 2) { // sp
        return (int)(value * WindowManager::getScaledDensity());
    }
    return value; // px or others
}

void LayoutInflater::parseViewAttributes(android::ResXMLParser* parser, std::shared_ptr<setu::view::View> view, ResourceManager* resManager, Theme* theme) {
    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        uint32_t resId = parser->getAttributeNameResID(i);
        
        if (attrName == "id" || resId == 0x010100d0) {
            // Check if it's a reference (e.g. @+id/...)
            int type = parser->getAttributeDataType(i);
            if (type == android::Res_value::TYPE_REFERENCE) {
                view->setId(parser->getAttributeData(i));
            }
        } else if (attrName == "visibility") {
            int val = parser->getAttributeData(i);
            if (val == 0) view->setVisibility(setu::view::View::VISIBLE);
            else if (val == 1) view->setVisibility(setu::view::View::INVISIBLE);
            else if (val == 2) view->setVisibility(setu::view::View::GONE);
        } else if (attrName == "padding" || resId == 0x010100d4) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setPadding(p, p, p, p);
        } else if (attrName == "paddingLeft" || attrName == "paddingStart" || resId == 0x010100d6 || resId == 0x010103b3) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setPadding(p, view->getPaddingTop(), view->getPaddingRight(), view->getPaddingBottom());
        } else if (attrName == "paddingTop" || resId == 0x010100d7) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setPadding(view->getPaddingLeft(), p, view->getPaddingRight(), view->getPaddingBottom());
        } else if (attrName == "paddingRight" || attrName == "paddingEnd" || resId == 0x010100d8 || resId == 0x010103b4) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setPadding(view->getPaddingLeft(), view->getPaddingTop(), p, view->getPaddingBottom());
        } else if (attrName == "paddingBottom" || resId == 0x010100d9) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setPadding(view->getPaddingLeft(), view->getPaddingTop(), view->getPaddingRight(), p);
        } else if (attrName == "minWidth" || resId == 0x0101013f) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setMinimumWidth(p);
        } else if (attrName == "minHeight" || resId == 0x01010140) {
            int type = parser->getAttributeDataType(i);
            int p = 0;
            if (type == android::Res_value::TYPE_DIMENSION) {
                p = parseComplexDimension(parser->getAttributeData(i));
            } else {
                size_t strLen = 0;
                const char16_t* str16 = parser->getAttributeStringValue(i, &strLen);
                if (str16) p = parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
            }
            view->setMinimumHeight(p);
        } else if (attrName == "gravity" || resId == 0x010100af) {
            view->setGravity(parser->getAttributeData(i));
        } else if (attrName == "background" || resId == 0x01010039) {
            int type = parser->getAttributeDataType(i);
            if (type >= android::Res_value::TYPE_FIRST_COLOR_INT && type <= android::Res_value::TYPE_LAST_COLOR_INT) {
                view->setBackgroundColor(parser->getAttributeData(i));
            } else if (type == android::Res_value::TYPE_REFERENCE || type == android::Res_value::TYPE_ATTRIBUTE) {
                // Resolve @drawable/@color references and ?attr/ theme attributes
                android::AssetManager2::SelectedValue val;
                val.type = type;
                val.data = parser->getAttributeData(i);
                val.cookie = android::kInvalidCookie;
                val.flags = 0;
                val.resid = 0;
                if (resManager && resManager->resolveValue(val, theme)) {
                    if (val.type >= android::Res_value::TYPE_FIRST_COLOR_INT &&
                        val.type <= android::Res_value::TYPE_LAST_COLOR_INT) {
                        view->setBackgroundColor(val.data);
                    }
                }
            }
        } else if (attrName == "clickable" || resId == 0x0101006e) {
            int type = parser->getAttributeDataType(i);
            if (type == android::Res_value::TYPE_INT_BOOLEAN) {
                view->setClickable(parser->getAttributeData(i) != 0);
            }
        } else if (attrName == "focusable" || resId == 0x0101006f) {
            int type = parser->getAttributeDataType(i);
            if (type == android::Res_value::TYPE_INT_BOOLEAN) {
                view->setFocusable(parser->getAttributeData(i) != 0);
            }
        }
    }
}

void LayoutInflater::parseLayoutParams(android::ResXMLParser* parser, std::shared_ptr<setu::view::View> view, std::shared_ptr<setu::view::ViewGroup> parent) {
    std::shared_ptr<setu::view::View::LayoutParams> lp;
    if (parent) {
        lp = parent->generateLayoutParams(parser);
    } else {
        lp = std::make_shared<setu::view::View::LayoutParams>(setu::view::View::WRAP_CONTENT, setu::view::View::WRAP_CONTENT);
    }

    auto parseDim = [&](size_t idx) {
        int type = parser->getAttributeDataType(idx);
        uint32_t data = parser->getAttributeData(idx);
        if (type == android::Res_value::TYPE_DIMENSION) {
            return LayoutInflater::parseComplexDimension(data);
        }
        // Also handle STRING dimensions like "16dp"
        size_t strLen = 0;
        const char16_t* str16 = parser->getAttributeStringValue(idx, &strLen);
        if (str16) {
            return parseDimension(android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen)));
        }
        return (int)data;
    };

    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        uint32_t resId = parser->getAttributeNameResID(i);
        
        if (attrName == "layout_width" || resId == 0x010100f4) {
            int type = parser->getAttributeDataType(i);
            uint32_t data = parser->getAttributeData(i);
            if (type == android::Res_value::TYPE_INT_DEC) {
                if (data == 0xFFFFFFFF) lp->width = setu::view::View::MATCH_PARENT;
                else if (data == 0xFFFFFFFE) lp->width = setu::view::View::WRAP_CONTENT;
                else lp->width = (int)data;
            } else {
                lp->width = parseDim(i);
            }
            continue;  // Important: skip to next attribute
        } else if (attrName == "layout_height" || resId == 0x010100f5) {
            int type = parser->getAttributeDataType(i);
            uint32_t data = parser->getAttributeData(i);
            if (type == android::Res_value::TYPE_INT_DEC) {
                if (data == 0xFFFFFFFF) lp->height = setu::view::View::MATCH_PARENT;
                else if (data == 0xFFFFFFFE) lp->height = setu::view::View::WRAP_CONTENT;
                else lp->height = (int)data;
            } else {
                lp->height = parseDim(i);
            }
            continue;
        } else if (attrName == "layout_marginLeft" || attrName == "layout_marginStart" || resId == 0x010100f7 || resId == 0x010103b1) {
            lp->leftMargin = parseDim(i);
        } else if (attrName == "layout_marginTop" || resId == 0x010100f8) {
            lp->topMargin = parseDim(i);
        } else if (attrName == "layout_marginRight" || attrName == "layout_marginEnd" || resId == 0x010100f9 || resId == 0x010103b2) {
            lp->rightMargin = parseDim(i);
        } else if (attrName == "layout_marginBottom" || resId == 0x010100fa) {
            lp->bottomMargin = parseDim(i);
        }
    }

    view->setLayoutParams(lp);
}

} // namespace setu



