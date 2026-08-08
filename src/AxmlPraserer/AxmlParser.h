#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "../utils/AndroidRes.h"

struct AxmlAttribute {
    std::string name;
    std::string rawValue; // For string types
    uint32_t typedValueData;
    uint8_t typedValueType;
};

struct AxmlNode {
    std::string tag;
    std::vector<AxmlAttribute> attributes;
    std::vector<std::unique_ptr<AxmlNode>> children;
};

// -----------------------------------------------------------------------------
// The AxmlParser Class
// -----------------------------------------------------------------------------
class AxmlParser {
public:
    AxmlParser();
    ~AxmlParser();

    // The main entry point. Pass the raw bytes you extracted from the APK here.
    bool parse(const std::vector<uint8_t>& axmlBuffer);

    // Get the parsed tree root
    const AxmlNode* getRootNode() const { return m_root.get(); }

private:
    // We will store all the parsed strings here so we can look them up later
    std::vector<std::string> m_stringPool;
    std::unique_ptr<AxmlNode> m_root;
    
    // Internal helper function to read the string pool chunk
    bool parseStringPool(const uint8_t* chunkStart);
};