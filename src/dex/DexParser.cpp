#include "DexParser.h"
#include <iostream>

using namespace std;

DexParser::DexParser() {
}

DexParser::~DexParser() {
}

bool DexParser::parse(const std::vector<uint8_t>& dexBuffer) {
    if (dexBuffer.size() < sizeof(DexHeader)) {
        cerr << "DEX buffer is too small to contain a header!" << endl;
        return false;
    }

    // Cast the start of the buffer to our DexHeader struct
    const DexHeader* header = reinterpret_cast<const DexHeader*>(dexBuffer.data());

    // 1. Verify Magic Number ("dex\n035\0" or "dex\n037\0" etc)
    // We just check the first 4 bytes for "dex\n" to be safe across versions.
    if (header->magic[0] != 'd' || header->magic[1] != 'e' || 
        header->magic[2] != 'x' || header->magic[3] != '\n') {
        cerr << "Invalid DEX file! Magic number mismatch." << endl;
        return false;
    }

    // Check Endianness (Android is almost always little-endian 0x12345678)
    if (header->endian_tag != 0x12345678) {
        cerr << "Warning: DEX file is not standard little-endian!" << endl;
    }

    cout << "\n=== DEX FILE HEADER LOADED ===" << endl;
    cout << "Version:         " << header->magic[4] << header->magic[5] << header->magic[6] << endl;
    cout << "Total Size:      " << header->file_size << " bytes" << endl;
    
    // Print out the table sizes that we need to parse next!
    cout << "\n--- ID Tables Summary ---" << endl;
    cout << "Strings:         " << header->string_ids_size << " items (Starts at offset: " << header->string_ids_off << ")" << endl;
    cout << "Types (Classes): " << header->type_ids_size << " items (Starts at offset: " << header->type_ids_off << ")" << endl;
    cout << "Methods:         " << header->method_ids_size << " items (Starts at offset: " << header->method_ids_off << ")" << endl;
    cout << "Class Defs:      " << header->class_defs_size << " items (Starts at offset: " << header->class_defs_off << ")" << endl;

    // --- 2. Parse String IDs ---
    // The string_ids_off points to an array of uint32_t offsets.
    const uint32_t* stringOffsets = reinterpret_cast<const uint32_t*>(dexBuffer.data() + header->string_ids_off);
    
    m_strings.reserve(header->string_ids_size);
    for (uint32_t i = 0; i < header->string_ids_size; ++i) {
        // Jump to where the actual string bytes are stored
        const uint8_t* strData = dexBuffer.data() + stringOffsets[i];
        
        // DEX strings are prefixed with their length in ULEB128 format
        uint32_t utf16Length = readUnsignedLeb128(&strData);
        
        // After the length, the characters follow as standard C-style null-terminated MUTF-8.
        // We can safely cast this straight to a C++ std::string!
        std::string s(reinterpret_cast<const char*>(strData));
        m_strings.push_back(s);
    }
    
    cout << "\nSuccessfully loaded " << m_strings.size() << " strings from DEX!" << endl;
    if (m_strings.size() > 0) {
        cout << "String[0]: " << m_strings[0] << endl;
        cout << "String[last]: " << m_strings.back() << endl;
    }

    // --- 3. Parse Type IDs ---
    // The type_ids table is just an array of type_id_item structs (each holds an index to the string pool)
    const type_id_item* typeIds = reinterpret_cast<const type_id_item*>(dexBuffer.data() + header->type_ids_off);
    
    // --- 4. Parse Method IDs ---
    // The method_ids table connects a class (from type_ids) to a method name (from string_ids)
    const method_id_item* methodIds = reinterpret_cast<const method_id_item*>(dexBuffer.data() + header->method_ids_off);
    
    cout << "\n--- METHOD DUMP (First 25) ---" << endl;
    for (uint32_t i = 0; i < header->method_ids_size; ++i) {
        if (i >= 25) break; // Only print the first 25 to avoid flooding the console
        
        const method_id_item& method = methodIds[i];
        
        // 1. Look up the class name: Method -> Class Index -> String Index
        uint32_t classNameStringIdx = typeIds[method.class_idx].descriptor_idx;
        std::string className = m_strings[classNameStringIdx];
        
        // 2. Look up the method name: Method -> String Index
        std::string methodName = m_strings[method.name_idx];
        
        cout << className << " -> " << methodName << "()" << endl;
    }

    return true;
}

// -----------------------------------------------------------------------------
// DEX uses ULEB128 (Unsigned Little-Endian Base 128) to compress integers.
// This saves space since small numbers only take 1 byte instead of 4.
// -----------------------------------------------------------------------------
uint32_t DexParser::readUnsignedLeb128(const uint8_t** pStream) {
    const uint8_t* ptr = *pStream;
    uint32_t result = *(ptr++);
    if (result > 0x7f) {
        uint32_t cur = *(ptr++);
        result = (result & 0x7f) | ((cur & 0x7f) << 7);
        if (cur > 0x7f) {
            cur = *(ptr++);
            result |= (cur & 0x7f) << 14;
            if (cur > 0x7f) {
                cur = *(ptr++);
                result |= (cur & 0x7f) << 21;
                if (cur > 0x7f) {
                    cur = *(ptr++);
                    result |= cur << 28;
                }
            }
        }
    }
    *pStream = ptr;
    return result;
}
