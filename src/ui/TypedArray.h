#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "../utils/AndroidRes.h"
#include "Theme.h"
#include "../AxmlPraserer/AxmlParser.h"

namespace windroid {

class TypedArray {
public:
    TypedArray(ResourceManager* resManager, const std::vector<uint32_t>& styleables);
    ~TypedArray();

    // The core 4-step attribute resolution algorithm.
    // Populates this TypedArray with values.
    void obtainStyledAttributes(const Theme* theme, const AxmlNode* node, uint32_t defStyleAttr, uint32_t defStyleRes);

    bool hasValue(int index) const;
    bool getBoolean(int index, bool defValue) const;
    int getInt(int index, int defValue) const;
    float getFloat(int index, float defValue) const;
    uint32_t getColor(int index, uint32_t defValue) const;
    int getDimensionPixelSize(int index, int defValue) const;
    std::string getString(int index) const;

private:
    ResourceManager* m_resManager;
    std::vector<uint32_t> m_styleables;
    std::vector<Res_value> m_values;
    std::vector<std::string> m_stringValues;
    std::vector<bool> m_hasValue;

    // Helper to find an attribute in the AXML node by its Resource ID
    const AxmlAttribute* findAttributeInNode(const AxmlNode* node, uint32_t attrResId) const;
};

} // namespace windroid
