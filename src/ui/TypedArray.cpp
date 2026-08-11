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

const AxmlAttribute* TypedArray::findAttributeInNode(const AxmlNode* node, uint32_t attrResId) const {
    for (const auto& attr : node->attributes) {
        if (attr.nameResId == attrResId) return &attr;
    }
    return nullptr;
}

void TypedArray::obtainStyledAttributes(const Theme* theme, const AxmlNode* node, uint32_t defStyleAttr, uint32_t defStyleRes) {
    // 1. Resolve style="..." attribute on the node
    uint32_t styleResId = 0;
    if (node) {
        for (const auto& attr : node->attributes) {
            if (attr.name == "style") {
                if (attr.typedValueType == 0x01) { // TYPE_REFERENCE
                    styleResId = attr.typedValueData;
                }
            }
        }
    }

    // 2. Resolve defStyleAttr against Theme
    uint32_t resolvedDefStyleRes = 0;
    if (defStyleAttr != 0 && theme) {
        Res_value val;
        if (theme->resolveAttribute(defStyleAttr, val) && val.dataType == 0x01) { // TYPE_REFERENCE
            resolvedDefStyleRes = val.data;
        }
    }

    const ArscParser::Bag* styleBag = styleResId ? m_resManager->getBag(styleResId) : nullptr;
    const ArscParser::Bag* defStyleAttrBag = resolvedDefStyleRes ? m_resManager->getBag(resolvedDefStyleRes) : nullptr;
    const ArscParser::Bag* defStyleResBag = defStyleRes ? m_resManager->getBag(defStyleRes) : nullptr;

    auto findInBagRecursive = [&](const ArscParser::Bag* b, uint32_t attrId, Res_value& out) -> bool {
        const ArscParser::Bag* current = b;
        int depth = 0;
        while (current && depth++ < 10) {
            for (const auto& m : current->maps) {
                if (m.name == attrId) {
                    out = m.value;
                    return true;
                }
            }
            if (current->parentResId != 0 && current->parentResId != 0xFFFFFFFF) {
                current = m_resManager->getBag(current->parentResId);
            } else {
                current = nullptr;
            }
        }
        return false;
    };

    for (size_t i = 0; i < m_styleables.size(); ++i) {
        uint32_t attrId = m_styleables[i];
        Res_value value;
        std::string rawString;
        bool found = false;

        // Step 1: Explicit XML attribute
        if (node) {
            const AxmlAttribute* attr = findAttributeInNode(node, attrId);
            if (attr) {
                value.dataType = attr->typedValueType;
                value.data = attr->typedValueData;
                rawString = attr->rawValue;
                found = true;
            }
        }

        // Step 2: Explicit style in XML
        if (!found && findInBagRecursive(styleBag, attrId, value)) found = true;

        // Step 3: Default style attribute (defStyleAttr)
        if (!found && findInBagRecursive(defStyleAttrBag, attrId, value)) found = true;

        // Step 4: Default style resource (defStyleRes)
        if (!found && findInBagRecursive(defStyleResBag, attrId, value)) found = true;

        // Step 5: Theme itself
        if (!found && theme && theme->resolveAttribute(attrId, value)) found = true;

        if (found) {
            // Resolve ?attr/ references if the found value is an attribute reference
            if (value.dataType == 0x02 && theme) {
                theme->resolveAttribute(value.data, value);
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
    if (type == 0x01) return defValue; // Unresolved reference
    return (int)m_values[index].data;
}

float TypedArray::getFloat(int index, float defValue) const {
    if (!hasValue(index)) return defValue;
    uint8_t type = m_values[index].dataType;
    if (type == 0x01) return defValue; // Unresolved reference
    
    // Basic float cast (actually should interpret bits if type is FLOAT)
    if (type == 0x04) { // TYPE_FLOAT
        union { uint32_t i; float f; } u;
        u.i = m_values[index].data;
        return u.f;
    }
    return (float)m_values[index].data;
}

uint32_t TypedArray::getColor(int index, uint32_t defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    if (type == 0x01) return defValue; // Unresolved reference
    if (type < 0x1c || type > 0x1f) return defValue; // Not a color type
    
    return m_values[index].data;
}

int TypedArray::getDimensionPixelSize(int index, int defValue) const {
    if (!hasValue(index)) return defValue;
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == 0x01) return defValue; // Unresolved reference
    
    if (type == 0x05) { // TYPE_DIMENSION
        int value = (int)(data >> 8);
        int unit = data & 0x0F;
        if (unit == 1 || unit == 2) { // dp or sp
            return value * 2; // naive 2.0 density
        }
        return value;
    }
    
    if (type >= 0x10 && type <= 0x1f) {
        return (int)data; // fallback for plain integers
    }
    
    return defValue;
}

std::string TypedArray::getString(int index) const {
    if (!hasValue(index)) return "";
    
    // If we extracted a literal string from AXML, return it directly
    if (!m_stringValues[index].empty()) {
        return m_stringValues[index];
    }
    
    uint8_t type = m_values[index].dataType;
    uint32_t data = m_values[index].data;
    
    if (type == 0x03 || type == 0x01) { // TYPE_STRING or TYPE_REFERENCE
        if (m_resManager) return m_resManager->getString(data);
    }
    
    return std::to_string(data);
}

} // namespace windroid
