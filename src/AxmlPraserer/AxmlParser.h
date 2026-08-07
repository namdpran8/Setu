#pragma once
#include <cstdint>
#include <vector>
#include <string>

// -----------------------------------------------------------------------------
// WHY WE USE #pragma pack(push, 1):
// By default, C++ compilers add invisible "padding" bytes between variables inside
// a struct to align them in memory for faster CPU access. 
// However, when we read binary files (like an APK or AXML), the bytes are tightly 
// packed with no padding. If we don't force the compiler to use 1-byte alignment, 
// our struct sizes won't match the file, and we'll read garbage data!
// -----------------------------------------------------------------------------
#pragma pack(push, 1)

// 1. The universal header. EVERY chunk in an AXML file starts with this.
// This tells us what we are currently looking at.
struct ResChunk_header {
    uint16_t type;       // What kind of chunk is this? (e.g., 0x0001 for String Pool)
    uint16_t headerSize; // How big is this specific header struct?
    uint32_t size;       // How big is the ENTIRE chunk (header + data)?
};

// 2. The String Pool header. 
// In AXML, text isn't stored directly inside the XML tags. Instead, all strings 
// are stored in one massive pool at the top of the file. The XML tags just use
// integer numbers to point to strings in this pool.
struct ResStringPool_header {
    ResChunk_header header; // Starts with the universal header
    uint32_t stringCount;   // Total number of strings in this pool
    uint32_t styleCount;    // Number of HTML-like styles (usually 0 in AndroidManifest)
    uint32_t flags;         // IMPORTANT: If (flags & 256) is true, strings are UTF-8. Else, UTF-16.
    uint32_t stringsStart;  // Byte offset from the start of this header to the string data
    uint32_t stylesStart;   // Byte offset to the style data
};

// 3. An XML Node header. 
// Used when an XML tag starts (<tag>) or ends (</tag>).
struct ResXMLTree_node {
    ResChunk_header header;
    uint32_t lineNumber;    // The line number where this tag appeared in the original text XML
    uint32_t comment;       // Index into string pool for a comment (0xFFFFFFFF means no comment)
};

// 4. Start Element Extension
// This follows immediately after a ResXMLTree_node if the type is 0x0102 (Start Element)
struct ResXMLTree_attrExt {
    uint32_t ns;              // Namespace string index
    uint32_t name;            // Tag name string index (e.g., "manifest", "activity")
    uint16_t attributeStart;  // Byte offset from start of this struct to the attributes
    uint16_t attributeSize;   // Size of one attribute struct
    uint16_t attributeCount;  // How many attributes this tag has
    uint16_t idIndex;
    uint16_t classIndex;
    uint16_t styleIndex;
};

// 5. XML Attribute
// Represents a single attribute (like package="com.example.hello")
struct ResXMLTree_attribute {
    uint32_t ns;              // Namespace string index
    uint32_t name;            // Attribute name string index (e.g., "package")
    uint32_t rawValue;        // Attribute string value index (e.g., "com.example.hello")
    
    // Sometimes the value isn't a string (it's a boolean or int). This holds that data.
    uint16_t typedValue_size;
    uint8_t  typedValue_res0;
    uint8_t  typedValue_dataType;
    uint32_t typedValue_data;
};

#pragma pack(pop) // Restore the compiler's default padding behavior for everything else


// -----------------------------------------------------------------------------
// The AxmlParser Class
// -----------------------------------------------------------------------------
class AxmlParser {
public:
    AxmlParser();
    ~AxmlParser();

    // The main entry point. Pass the raw bytes you extracted from the APK here.
    bool parse(const std::vector<uint8_t>& axmlBuffer);

private:
    // We will store all the parsed strings here so we can look them up later
    std::vector<std::string> m_stringPool;
    
    // Internal helper function to read the string pool chunk
    bool parseStringPool(const uint8_t* chunkStart);
};