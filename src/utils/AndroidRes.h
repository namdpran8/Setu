#pragma once
#include <cstdint>

// -----------------------------------------------------------------------------
// WHY WE USE #pragma pack(push, 1):
// By default, C++ compilers add invisible "padding" bytes between variables inside
// a struct to align them in memory for faster CPU access. 
// However, when we read binary files (like an APK or AXML), the bytes are tightly 
// packed with no padding. If we don't force the compiler to use 1-byte alignment, 
// our struct sizes won't match the file, and we'll read garbage data!
// -----------------------------------------------------------------------------
#pragma pack(push, 1)

// 1. The universal header. EVERY chunk in an AXML or ARSC file starts with this.
struct ResChunk_header {
    uint16_t type;       // What kind of chunk is this?
    uint16_t headerSize; // How big is this specific header struct?
    uint32_t size;       // How big is the ENTIRE chunk (header + data)?
};

// 2. The String Pool header. 
struct ResStringPool_header {
    ResChunk_header header; 
    uint32_t stringCount;   
    uint32_t styleCount;    
    uint32_t flags;         
    uint32_t stringsStart;  
    uint32_t stylesStart;   
};

// --- ARSC Specific Structs ---

struct ResTable_header {
    ResChunk_header header;
    uint32_t packageCount;
};

struct ResTable_package {
    ResChunk_header header;
    uint32_t id;
    char16_t name[128];
    uint32_t typeStrings;
    uint32_t lastPublicType;
    uint32_t keyStrings;
    uint32_t lastPublicKey;
    uint32_t typeIdOffset;
};

struct ResTable_typeSpec {
    ResChunk_header header;
    uint8_t id;
    uint8_t res0;
    uint16_t res1;
    uint32_t entryCount;
};

struct ResTable_type {
    ResChunk_header header;
    uint8_t id;
    uint8_t res0;
    uint16_t res1;
    uint32_t entryCount;
    uint32_t entriesStart;
};

struct ResTable_entry {
    uint16_t size;
    uint16_t flags;
    uint32_t key;
};

struct Res_value {
    uint16_t size;
    uint8_t res0;
    uint8_t dataType;
    uint32_t data;
};

// --- AXML Specific Structs ---

struct ResXMLTree_node {
    ResChunk_header header;
    uint32_t lineNumber;    
    uint32_t comment;       
};

struct ResXMLTree_attrExt {
    uint32_t ns;              
    uint32_t name;            
    uint16_t attributeStart;  
    uint16_t attributeSize;   
    uint16_t attributeCount;  
    uint16_t idIndex;
    uint16_t classIndex;
    uint16_t styleIndex;
};

struct ResXMLTree_attribute {
    uint32_t ns;              
    uint32_t name;            
    uint32_t rawValue;        
    
    uint16_t typedValue_size;
    uint8_t  typedValue_res0;
    uint8_t  typedValue_dataType;
    uint32_t typedValue_data;
};

#pragma pack(pop)
