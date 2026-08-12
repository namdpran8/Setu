#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "androidfw/ResourceTypes.h"
#include "androidfw/AssetManager2.h"
#include "Theme.h"

namespace windroid {

class TypedArray {
public:
    TypedArray(ResourceManager* resManager, const std::vector<uint32_t>& styleables);
    ~TypedArray();

    // Populates this TypedArray with values.
    void obtainStyledAttributes(const Theme* theme, android::ResXMLParser* parser, uint32_t defStyleAttr, uint32_t defStyleRes);

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
    std::vector<android::Res_value> m_values;
    std::vector<std::string> m_stringValues;
    std::vector<bool> m_hasValue;
};

} // namespace windroid

