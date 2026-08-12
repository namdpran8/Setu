#include "TypedArray.h"
#include "../ui/LayoutInflater.h"
#include "../utils/Logger.h"

namespace windroid {

TypedArray::TypedArray(ResourceManager* resManager, const std::vector<uint32_t>& styleables)
    : m_resManager(resManager), m_styleables(styleables) {
    m_values.resize(styleables.size());
    m_stringValues.resize(styleables.size());
    m_hasValue.resize(styleables.size(), false);
}

TypedArray::~TypedArray() {
}

void TypedArray::obtainStyledAttributes(const Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes) {
    for (size_t i = 0; i < m_styleables.size(); ++i) {
        uint32_t attrId = m_styleables[i];
        android::Res_value value;
        std::string rawString;
        bool found = false;

        // Step 1: Explicit XML attribute
        if (parser) {
            for (size_t j = 0; j < parser->getAttributeCount(); j++) {
                if (parser->getAttributeNameResID(j) == attrId) {
                    value.dataType = parser->getAttributeDataType(j);
                    value.data = parser->getAttributeData(j);
                    found = true;
                    
                    if (value.dataType == android::Res_value::TYPE_STRING) {
                        size_t strLen;
                        const char16_t* str16 = parser->getAttributeStringValue(j, &strLen);
                        if (str16) {
                            rawString = android::util::Utf16ToUtf8(android::StringPiece16(str16, strLen));
                        }
                    }
                    break;
                }
            }
        }

        // Step 2: Theme
        if (!found && theme && theme->getTheme()) {
            auto optVal = theme->getTheme()->GetAttribute(attrId);
            if (optVal.has_value()) {
                value.dataType = optVal->type;
                value.data = optVal->data;
                found = true;
            }
        }

        if (found) {
            // Resolve ?attr/ references if the found value is an attribute reference
            if (value.dataType == android::Res_value::TYPE_ATTRIBUTE && theme && theme->getTheme()) {
                std::optional<android::AssetManager2::SelectedValue> optVal = theme->getTheme()->GetAttribute(value.data);
                if (optVal.has_value()) {
                    value.dataType = optVal->type;
                    value.data = optVal->data;
                }
            }
            m_values[i] = value;
            m_stringValues[i] = rawString;
            m_hasValue[i] = true;
        }
    }
}

bool TypedArray::hasValue(int index) const {
    if (index < 0 || index >= m_hasValue.size()) return false;
    return m_hasValue[index];
}

bool TypedArray::getBoolean(int index, bool defValue) const {
    if (!hasValue(index)) return defValue;
    return m_values[index].data != 0;
}

int TypedArray::getInt(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    if (type == android::Res_value::TYPE_REFERENCE) return defValue; // Unresolved reference
    return (int)m_values[index].data;
}

float TypedArray::getFloat(int index, float defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    if (type == android::Res_value::TYPE_REFERENCE) return defValue;
    
    if (type == android::Res_value::TYPE_FLOAT) {
        union { uint32_t i; float f; } u;
        u.i = m_values[index].data;
        return u.f;
    }
    return (float)m_values[index].data;
}

uint32_t TypedArray::getColor(int index, uint32_t defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    if (type == android::Res_value::TYPE_REFERENCE) return defValue;
    if (type < android::Res_value::TYPE_FIRST_COLOR_INT || type > android::Res_value::TYPE_LAST_COLOR_INT) return defValue;
    
    return m_values[index].data;
}

int TypedArray::getDimensionPixelSize(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == android::Res_value::TYPE_REFERENCE) return defValue;
    
    if (type == android::Res_value::TYPE_DIMENSION) {
        int value = (int)(data >> 8);
        int unit = data & 0x0F;
        if (unit == 1 || unit == 2) { // dp or sp
            return value * 2; // naive 2.0 density
        }
        return value;
    }
    
    if (type >= android::Res_value::TYPE_FIRST_INT && type <= android::Res_value::TYPE_LAST_INT) {
        return (int)data; // fallback for plain integers
    }
    
    return defValue;
}

std::string TypedArray::getString(int index) const {
    if (!hasValue(index)) return "";
    
    if (!m_stringValues[index].empty()) {
        return m_stringValues[index];
    }
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == android::Res_value::TYPE_STRING || type == android::Res_value::TYPE_REFERENCE) {
        if (m_resManager) return m_resManager->getString(data);
    }
    
    return std::to_string(data);
}

} // namespace windroid

