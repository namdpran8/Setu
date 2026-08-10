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

std::shared_ptr<windroid::view::View> LayoutInflater::inflate(const AxmlNode* node, ResourceManager* resManager) {
    if (!node) return nullptr;
    return inflateRecursive(node, resManager);
}

std::shared_ptr<windroid::view::View> LayoutInflater::inflateRecursive(const AxmlNode* node, ResourceManager* resManager) {
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
    } else if (tag.find("LinearLayout") != std::string::npos) {
        auto ll = std::make_shared<windroid::view::LinearLayout>();
        
        // Check orientation
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
        view = ll;
    } else if (tag.find("FrameLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::FrameLayout>();
    } else if (tag.find("RelativeLayout") != std::string::npos) {
        view = std::make_shared<windroid::view::RelativeLayout>();
    } else if (tag.find("TextView") != std::string::npos) {
        auto tv = std::make_shared<windroid::widget::TextView>();
        for (const auto& attr : node->attributes) {
            if (attr.name == "text") {
                tv->setText(utf8_to_utf16(resolveString(attr, resManager)));
            } else if (attr.name == "textSize") {
                tv->setTextSize((float)parseDimension(resolveString(attr, resManager)));
            } else if (attr.name == "textColor") {
                // Simplistic color parsing
                uint32_t color = 0xFF000000;
                if (attr.typedValueType >= 0x1c && attr.typedValueType <= 0x1f) {
                    color = attr.typedValueData;
                }
                tv->setTextColor(color);
            }
        }
        view = tv;
    } else if (tag.find("Button") != std::string::npos) {
        auto btn = std::make_shared<windroid::widget::Button>();
        for (const auto& attr : node->attributes) {
            if (attr.name == "text") {
                btn->setText(utf8_to_utf16(resolveString(attr, resManager)));
            }
        }
        view = btn;
    } else if (tag.find("EditText") != std::string::npos) {
        auto et = std::make_shared<windroid::widget::EditText>();
        for (const auto& attr : node->attributes) {
            if (attr.name == "text" || attr.name == "hint") {
                et->setText(utf8_to_utf16(resolveString(attr, resManager)));
            }
        }
        view = et;
    } else if (tag == "Guideline") {
        view = std::make_shared<windroid::view::View>();
    } else {
        Logger::w("LayoutInflater", "Unsupported view tag: " + tag + ", falling back to View");
        view = std::make_shared<windroid::view::View>();
    }

    parseViewAttributes(node, view, resManager);

    // Parse children if it is a ViewGroup
    auto viewGroup = std::dynamic_pointer_cast<windroid::view::ViewGroup>(view);
    if (viewGroup) {
        for (const auto& childNode : node->children) {
            auto childView = inflateRecursive(childNode.get(), resManager);
            if (childView) {
                parseLayoutParams(childNode.get(), childView, tag);
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

void LayoutInflater::parseViewAttributes(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, ResourceManager* resManager) {
    for (const auto& attr : node->attributes) {
        if (attr.name == "id") {
            view->setId(attr.typedValueData);
        }
    }
}

void LayoutInflater::parseLayoutParams(const AxmlNode* node, std::shared_ptr<windroid::view::View> view, const std::string& parentTag) {
    std::shared_ptr<windroid::view::View::LayoutParams> lp;

    // Instantiate correct LayoutParams based on parent
    if (parentTag.find("ConstraintLayout") != std::string::npos) {
        lp = std::make_shared<windroid::view::ConstraintLayout::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    } else if (parentTag.find("GridLayout") != std::string::npos) {
        lp = std::make_shared<windroid::view::GridLayout::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    } else if (parentTag.find("LinearLayout") != std::string::npos) {
        lp = std::make_shared<windroid::view::LinearLayout::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    } else if (parentTag.find("FrameLayout") != std::string::npos) {
        lp = std::make_shared<windroid::view::FrameLayout::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    } else if (parentTag.find("RelativeLayout") != std::string::npos) {
        lp = std::make_shared<windroid::view::RelativeLayout::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    } else {
        lp = std::make_shared<windroid::view::View::LayoutParams>(windroid::view::View::WRAP_CONTENT, windroid::view::View::WRAP_CONTENT);
    }

    for (const auto& attr : node->attributes) {
        if (attr.name == "layout_width") {
            if (attr.typedValueData == 0xFFFFFFFF) lp->width = windroid::view::View::MATCH_PARENT;
            else if (attr.typedValueData == 0xFFFFFFFE) lp->width = windroid::view::View::WRAP_CONTENT;
            else if (attr.typedValueType == 0x10) lp->width = attr.typedValueData; // int
            else if (attr.typedValueType == 0x05) lp->width = (attr.typedValueData >> 8) * 2; // naive dp parsing
        } else if (attr.name == "layout_height") {
            if (attr.typedValueData == 0xFFFFFFFF) lp->height = windroid::view::View::MATCH_PARENT;
            else if (attr.typedValueData == 0xFFFFFFFE) lp->height = windroid::view::View::WRAP_CONTENT;
            else if (attr.typedValueType == 0x10) lp->height = attr.typedValueData;
            else if (attr.typedValueType == 0x05) lp->height = (attr.typedValueData >> 8) * 2;
        } else if (attr.name == "layout_marginLeft" || attr.name == "layout_marginStart") {
            lp->leftMargin = (attr.typedValueType == 0x05) ? (attr.typedValueData >> 8) * 2 : attr.typedValueData;
        } else if (attr.name == "layout_marginTop") {
            lp->topMargin = (attr.typedValueType == 0x05) ? (attr.typedValueData >> 8) * 2 : attr.typedValueData;
        } else if (attr.name == "layout_marginRight" || attr.name == "layout_marginEnd") {
            lp->rightMargin = (attr.typedValueType == 0x05) ? (attr.typedValueData >> 8) * 2 : attr.typedValueData;
        } else if (attr.name == "layout_marginBottom") {
            lp->bottomMargin = (attr.typedValueType == 0x05) ? (attr.typedValueData >> 8) * 2 : attr.typedValueData;
        }
        
        // LinearLayout specific
        auto llp = std::dynamic_pointer_cast<windroid::view::LinearLayout::LayoutParams>(lp);
        if (llp) {
            if (attr.name == "layout_weight") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                llp->weight = (attr.typedValueType == 0x04) ? u.f : (float)attr.typedValueData;
            } else if (attr.name == "layout_gravity") {
                llp->gravity = attr.typedValueData;
            }
        }
        
        // FrameLayout specific
        auto flp = std::dynamic_pointer_cast<windroid::view::FrameLayout::LayoutParams>(lp);
        if (flp) {
            if (attr.name == "layout_gravity") {
                flp->gravity = attr.typedValueData;
            }
        }
        
        // RelativeLayout specific
        auto rlp = std::dynamic_pointer_cast<windroid::view::RelativeLayout::LayoutParams>(lp);
        if (rlp) {
            if (attr.name == windroid::view::RelativeLayout::LayoutParams::ABOVE ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::BELOW ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::LEFT_OF ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::RIGHT_OF ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::ALIGN_PARENT_LEFT ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::ALIGN_PARENT_TOP ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::ALIGN_PARENT_RIGHT ||
                attr.name == windroid::view::RelativeLayout::LayoutParams::ALIGN_PARENT_BOTTOM) {
                rlp->rules[attr.name] = attr.typedValueData;
            }
        }
        
        // ConstraintLayout specific
        auto clp = std::dynamic_pointer_cast<windroid::view::ConstraintLayout::LayoutParams>(lp);
        if (clp) {
            if (node->tag == "Guideline") clp->isGuideline = true;
            if (attr.name == "layout_constraintTop_toTopOf") clp->topToTop = attr.typedValueData;
            else if (attr.name == "layout_constraintTop_toBottomOf") clp->topToBottom = attr.typedValueData;
            else if (attr.name == "layout_constraintBottom_toTopOf") clp->bottomToTop = attr.typedValueData;
            else if (attr.name == "layout_constraintBottom_toBottomOf") clp->bottomToBottom = attr.typedValueData;
            else if (attr.name == "layout_constraintStart_toStartOf" || attr.name == "layout_constraintLeft_toLeftOf") clp->startToStart = attr.typedValueData;
            else if (attr.name == "layout_constraintStart_toEndOf" || attr.name == "layout_constraintLeft_toRightOf") clp->startToEnd = attr.typedValueData;
            else if (attr.name == "layout_constraintEnd_toStartOf" || attr.name == "layout_constraintRight_toLeftOf") clp->endToStart = attr.typedValueData;
            else if (attr.name == "layout_constraintEnd_toEndOf" || attr.name == "layout_constraintRight_toRightOf") clp->endToEnd = attr.typedValueData;
            else if (attr.name == "layout_constraintHorizontal_bias") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                clp->horizontalBias = u.f;
            }
            else if (attr.name == "layout_constraintVertical_bias") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                clp->verticalBias = u.f;
            }
            else if (attr.name == "layout_constraintGuide_percent") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                clp->guidePercent = u.f;
            }
            else if (attr.name == "layout_constraintGuide_begin") clp->guideBegin = (attr.typedValueData >> 8) * 2;
            else if (attr.name == "layout_constraintGuide_end") clp->guideEnd = (attr.typedValueData >> 8) * 2;
            else if (attr.name == "orientation") clp->orientation = attr.typedValueData;
        }
        
        // GridLayout specific
        auto glp = std::dynamic_pointer_cast<windroid::view::GridLayout::LayoutParams>(lp);
        if (glp) {
            if (attr.name == "layout_column") glp->columnSpec.spanStart = attr.typedValueData;
            else if (attr.name == "layout_row") glp->rowSpec.spanStart = attr.typedValueData;
            else if (attr.name == "layout_columnSpan") glp->columnSpec.spanSize = attr.typedValueData;
            else if (attr.name == "layout_rowSpan") glp->rowSpec.spanSize = attr.typedValueData;
            else if (attr.name == "layout_columnWeight") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                glp->columnSpec.weight = (attr.typedValueType == 0x04) ? u.f : (float)attr.typedValueData;
            }
            else if (attr.name == "layout_rowWeight") {
                union { uint32_t i; float f; } u;
                u.i = attr.typedValueData;
                glp->rowSpec.weight = (attr.typedValueType == 0x04) ? u.f : (float)attr.typedValueData;
            }
            else if (attr.name == "layout_gravity") {
                int gravity = attr.typedValueData;
                if ((gravity & 0x7) == 0x7) glp->columnSpec.alignment = windroid::view::GridLayout::FILL;
                else if ((gravity & 0x1) == 0x1) glp->columnSpec.alignment = windroid::view::GridLayout::CENTER;
                else if ((gravity & 0x5) == 0x5 || (gravity & 0x800005) == 0x800005) glp->columnSpec.alignment = windroid::view::GridLayout::END;
                else glp->columnSpec.alignment = windroid::view::GridLayout::START;
                
                if ((gravity & 0x70) == 0x70) glp->rowSpec.alignment = windroid::view::GridLayout::FILL;
                else if ((gravity & 0x10) == 0x10) glp->rowSpec.alignment = windroid::view::GridLayout::CENTER;
                else if ((gravity & 0x50) == 0x50 || (gravity & 0x800050) == 0x800050) glp->rowSpec.alignment = windroid::view::GridLayout::END;
                else glp->rowSpec.alignment = windroid::view::GridLayout::START;
            }
        }
    }

    view->setLayoutParams(lp);
}

} // namespace windroid
