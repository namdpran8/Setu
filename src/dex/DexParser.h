#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "../interpreter/Value.h"

// Ensure 1-byte alignment just like AXML!
#pragma pack(push, 1)

// This perfectly matches the "header_item" from the AOSP documentation
struct DexHeader {
    uint8_t  magic[8];       // "dex\n035\0"
    uint32_t checksum;       // adler32 of the rest of the file
    uint8_t  signature[20];  // SHA-1 signature of the rest of the file
    uint32_t file_size;      // total file size in bytes
    uint32_t header_size;    // always 112 bytes
    uint32_t endian_tag;     // 0x12345678 (used to detect endianness)
    uint32_t link_size;
    uint32_t link_off;
    uint32_t map_off;        // offset to the map list
    
    // --- The ID Tables (The lists we need to parse!) ---
    uint32_t string_ids_size; // How many strings are in the file
    uint32_t string_ids_off;  // Where the string IDs start

    uint32_t type_ids_size;   // How many classes/types
    uint32_t type_ids_off;

    uint32_t proto_ids_size;
    uint32_t proto_ids_off;

    uint32_t field_ids_size;
    uint32_t field_ids_off;

    uint32_t method_ids_size; // How many methods
    uint32_t method_ids_off;

    uint32_t class_defs_size; // How many actual classes are implemented here
    uint32_t class_defs_off;

    uint32_t data_size;
    uint32_t data_off;
};

// Maps a class type (like "Ljava/lang/String;") to a string in the string pool
struct type_id_item {
    uint32_t descriptor_idx; // Index into string_ids
};

// Represents a method (like "toString") attached to a specific class type
struct method_id_item {
    uint16_t class_idx;      // Index into type_ids
    uint16_t proto_idx;      // Index into proto_ids (return types/args)
    uint32_t name_idx;       // Index into string_ids (the actual method name)
};

struct proto_id_item {
    uint32_t shorty_idx;
    uint32_t return_type_idx;
    uint32_t parameters_off;
};

// Note: type_list is variable size, so we don't define the array here
struct type_list {
    uint32_t size;
    // type_item list[size]; (where type_item is just a uint16_t type_idx)
};

struct class_def_item {
    uint32_t class_idx;
    uint32_t access_flags;
    uint32_t superclass_idx;
    uint32_t interfaces_off;
    uint32_t source_file_idx;
    uint32_t annotations_off;
    uint32_t class_data_off;
    uint32_t static_values_off;
};

struct code_item_header {
    uint16_t registers_size;
    uint16_t ins_size;
    uint16_t outs_size;
    uint16_t tries_size;
    uint32_t debug_info_off;
    uint32_t insns_size;
};

struct field_id_item {
    uint16_t class_idx;
    uint16_t type_idx;
    uint32_t name_idx;
};

#pragma pack(pop)

class DexParser {
public:
    DexParser();
    ~DexParser();

    bool parse(const std::vector<uint8_t>& dexBuffer);

    // Look up a method's full signature by its ID (e.g. "com.pranshu.test1.MainActivity -> setContentView")
    std::string getMethodSignature(uint32_t methodIdx) const;

    // Dynamically extracts the Dalvik bytecode for a specific class and method
    std::vector<uint8_t> getMethodBytecode(const std::string& className, const std::string& methodName) const;

    // Looks up the initial value of a static field
    Value getStaticFieldValue(uint32_t fieldIdx) const;
    
    // Looks up the initial value of a static field by string name
    Value getStaticFieldValueByName(const std::string& className, const std::string& fieldName) const;
    
    // String resolution helpers for fields
    std::string getClassNameFromFieldIdx(uint32_t fieldIdx) const;
    std::string getFieldNameFromFieldIdx(uint32_t fieldIdx) const;

private:
    std::vector<std::string> m_strings;
    
    // Pointers to DEX structures in memory
    const uint8_t* m_dexBufferStart = nullptr;
    const type_id_item* m_typeIds = nullptr;
    const proto_id_item* m_protoIds = nullptr;
    const field_id_item* m_fieldIds = nullptr;
    uint32_t m_fieldIdsSize = 0;
    const method_id_item* m_methodIds = nullptr;
    uint32_t m_methodIdsSize = 0;
    const class_def_item* m_classDefs = nullptr;
    uint32_t m_classDefsSize = 0;

    // Helper to read Android's custom variable-length integers
    uint32_t readUnsignedLeb128(const uint8_t** pStream) const;
    Value readEncodedValue(const uint8_t** pStream) const;
};
