#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include "../utils/AndroidRes.h"

class ArscParser {
public:
    ArscParser();
    ~ArscParser();

    bool parse(const std::vector<uint8_t>& buffer);

    // Resolves a resource ID (e.g. 0x7f0b001c) to its string value (e.g. "res/layout/activity_main.xml")
    std::string resolveStringValue(uint32_t resId) const;

    struct Bag {
        uint32_t parentResId;
        std::vector<ResTable_map> maps;
    };

    // Get a complex bag by resource ID
    const Bag* getBag(uint32_t resId) const;

private:
    void parseStringPool(const uint8_t* ptr, std::vector<std::string>& outStrings);
    void parsePackage(const uint8_t* ptr);

    std::vector<std::string> m_globalStrings;
    std::vector<std::string> m_typeStrings;
    std::vector<std::string> m_keyStrings;

    // Maps a resource ID (0xPPTTEEEE) to a global string pool index
    std::unordered_map<uint32_t, uint32_t> m_resourceStringPoolIndices;

    // Maps a resource ID to its complex bag (style/array)
    std::unordered_map<uint32_t, Bag> m_bags;
};
