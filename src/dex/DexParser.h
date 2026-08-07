#pragma once
#include <cstdint>
#include <vector>
#include <string>

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

#pragma pack(pop)

class DexParser {
public:
    DexParser();
    ~DexParser();

    bool parse(const std::vector<uint8_t>& dexBuffer);

private:
    std::vector<std::string> m_strings;

    // Helper to read Android's custom variable-length integers
    uint32_t readUnsignedLeb128(const uint8_t** pStream);
};
