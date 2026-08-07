#include "DexParser.h"
#include <iostream>
#include <iostream>
#include "../utils/Logger.h"
using namespace std;

DexParser::DexParser() {
}

DexParser::~DexParser() {
}

bool DexParser::parse(const std::vector<uint8_t>& dexBuffer) {
    if (dexBuffer.size() < sizeof(DexHeader)) {
        Logger::e("DexParser", "DEX buffer is too small to contain a header!");
        return false;
    }

    m_dexBufferStart = dexBuffer.data();

    // Cast the start of the buffer to our DexHeader struct
    const DexHeader* header = reinterpret_cast<const DexHeader*>(dexBuffer.data());

    // 1. Verify Magic Number ("dex\n035\0" or "dex\n037\0" etc)
    // We just check the first 4 bytes for "dex\n" to be safe across versions.
    if (header->magic[0] != 'd' || header->magic[1] != 'e' || 
        header->magic[2] != 'x' || header->magic[3] != '\n') {
        Logger::e("DexParser", "Invalid DEX file! Magic number mismatch.");
        return false;
    }

    // Check Endianness (Android is almost always little-endian 0x12345678)
    if (header->endian_tag != 0x12345678) {
        Logger::w("DexParser", "Warning: DEX file is not standard little-endian!");
    }

    Logger::i("DexParser", "=== DEX FILE HEADER LOADED ===");
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
    
    // Cleaned up the method dump that used to be here
    
    Logger::i("DexParser", "Successfully loaded " + std::to_string(m_strings.size()) + " strings from DEX!");
    if (m_strings.size() > 0) {
        cout << "String[0]: " << m_strings[0] << endl;
        cout << "String[last]: " << m_strings.back() << endl;
    }

    // --- 3. Parse Type IDs ---
    m_typeIds = reinterpret_cast<const type_id_item*>(dexBuffer.data() + header->type_ids_off);
    
    // --- Parse Proto IDs ---
    m_protoIds = reinterpret_cast<const proto_id_item*>(dexBuffer.data() + header->proto_ids_off);
    
    // --- Parse Field IDs ---
    m_fieldIdsSize = header->field_ids_size;
    m_fieldIds = reinterpret_cast<const field_id_item*>(dexBuffer.data() + header->field_ids_off);
    
    // --- 4. Parse Method IDs ---
    m_methodIdsSize = header->method_ids_size;
    m_methodIds = reinterpret_cast<const method_id_item*>(dexBuffer.data() + header->method_ids_off);
    
    // --- 5. Parse Class Defs ---
    m_classDefsSize = header->class_defs_size;
    m_classDefs = reinterpret_cast<const class_def_item*>(dexBuffer.data() + header->class_defs_off);
    
    cout << "\n--- METHOD DUMP (First 25) ---" << endl;
    for (uint32_t i = 0; i < header->method_ids_size; ++i) {
        if (i >= 25) break; // Only print the first 25 to avoid flooding the console
        
        const method_id_item& method = m_methodIds[i];
        
        // 1. Look up the class name: Method -> Class Index -> String Index
        uint32_t classNameStringIdx = m_typeIds[method.class_idx].descriptor_idx;
        std::string className = m_strings[classNameStringIdx];
        
        // 2. Look up the method name: Method -> String Index
        std::string methodName = m_strings[method.name_idx];
        
        cout << className << " -> " << methodName << "()" << endl;
    }

    return true;
}

std::string DexParser::getMethodSignature(uint32_t methodIdx) const {
    if (methodIdx >= m_methodIdsSize || !m_typeIds || !m_methodIds || !m_protoIds) {
        return "<unknown_method>";
    }
    
    const method_id_item& method = m_methodIds[methodIdx];
    uint32_t classNameStringIdx = m_typeIds[method.class_idx].descriptor_idx;
    std::string className = m_strings[classNameStringIdx];
    std::string methodName = m_strings[method.name_idx];
    
    // 3. Look up proto
    const proto_id_item& proto = m_protoIds[method.proto_idx];
    std::string returnType = m_strings[m_typeIds[proto.return_type_idx].descriptor_idx];
    
    std::string params = "";
    if (proto.parameters_off != 0 && m_dexBufferStart != nullptr) {
        const type_list* typeList = reinterpret_cast<const type_list*>(m_dexBufferStart + proto.parameters_off);
        const uint16_t* listStart = reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(typeList) + 4);
        for (uint32_t i = 0; i < typeList->size; ++i) {
            uint16_t typeIdx = listStart[i];
            params += m_strings[m_typeIds[typeIdx].descriptor_idx];
        }
    }
    
    // Format: Lclass;->method(Lparams;)V
    return className + "->" + methodName + "(" + params + ")" + returnType;
}

std::vector<uint8_t> DexParser::getMethodBytecode(const std::string& className, const std::string& methodName) const {
    if (!m_classDefs || !m_dexBufferStart) {
        return {};
    }

    for (uint32_t i = 0; i < m_classDefsSize; ++i) {
        const class_def_item& classDef = m_classDefs[i];
        
        // Match class name
        uint32_t typeStringIdx = m_typeIds[classDef.class_idx].descriptor_idx;
        std::string currentClassName = m_strings[typeStringIdx];
        if (currentClassName != className) {
            continue;
        }

        // Found the class!
        if (classDef.class_data_off == 0) {
            Logger::w("DexParser", "Class " + className + " has no class_data_item (marker interface?)");
            return {};
        }

        const uint8_t* ptr = m_dexBufferStart + classDef.class_data_off;
        
        // Read lengths
        uint32_t staticFieldsSize = readUnsignedLeb128(&ptr);
        uint32_t instanceFieldsSize = readUnsignedLeb128(&ptr);
        uint32_t directMethodsSize = readUnsignedLeb128(&ptr);
        uint32_t virtualMethodsSize = readUnsignedLeb128(&ptr);

        // Skip static fields
        for (uint32_t f = 0; f < staticFieldsSize; ++f) {
            readUnsignedLeb128(&ptr); // field_idx_diff
            readUnsignedLeb128(&ptr); // access_flags
        }

        // Skip instance fields
        for (uint32_t f = 0; f < instanceFieldsSize; ++f) {
            readUnsignedLeb128(&ptr);
            readUnsignedLeb128(&ptr);
        }

        // Helper lambda to search a method list
        auto searchMethodList = [&](uint32_t methodCount) -> std::vector<uint8_t> {
            uint32_t methodIdx = 0; // Reset prev_method_idx to 0 for each new list!
            for (uint32_t m = 0; m < methodCount; ++m) {
                uint32_t methodIdxDiff = readUnsignedLeb128(&ptr);
                uint32_t accessFlags = readUnsignedLeb128(&ptr);
                uint32_t codeOff = readUnsignedLeb128(&ptr);

                methodIdx += methodIdxDiff; // Add diff to running total

                std::string currentMethodName = m_strings[m_methodIds[methodIdx].name_idx];
                if (currentMethodName == methodName) {
                    // Found the method!
                    if (codeOff == 0) {
                        Logger::w("DexParser", "Method " + methodName + " has code_off == 0 (abstract or native)");
                        return {};
                    }

                    const uint8_t* codeItemPtr = m_dexBufferStart + codeOff;
                    const code_item_header* codeHeader = reinterpret_cast<const code_item_header*>(codeItemPtr);
                    
                    // The insns array starts exactly 16 bytes after the start of the code_item
                    const uint16_t* insnsPtr = reinterpret_cast<const uint16_t*>(codeItemPtr + 16);
                    
                    // insns_size is in 16-bit code units, so multiply by 2 for bytes
                    uint32_t bytecodeBytes = codeHeader->insns_size * 2;
                    
                    std::vector<uint8_t> bytecode;
                    bytecode.reserve(bytecodeBytes);
                    const uint8_t* rawInsns = reinterpret_cast<const uint8_t*>(insnsPtr);
                    for (uint32_t b = 0; b < bytecodeBytes; ++b) {
                        bytecode.push_back(rawInsns[b]);
                    }
                    
                    Logger::i("DexParser", "Dynamically extracted " + std::to_string(bytecodeBytes) + " bytes of bytecode for " + className + "." + methodName);
                    return bytecode;
                }
            }
            return {}; // Not found in this list
        };

        // Search direct methods first
        std::vector<uint8_t> result = searchMethodList(directMethodsSize);
        if (!result.empty()) return result;

        // Search virtual methods second
        result = searchMethodList(virtualMethodsSize);
        if (!result.empty()) return result;

        Logger::w("DexParser", "Method " + methodName + " not found in class " + className);
        return {};
    }

    Logger::w("DexParser", "Class " + className + " not found in DEX.");
    return {};
}

// -----------------------------------------------------------------------------
// DEX uses ULEB128 (Unsigned Little-Endian Base 128) to compress integers.
// This saves space since small numbers only take 1 byte instead of 4.
// -----------------------------------------------------------------------------
uint32_t DexParser::readUnsignedLeb128(const uint8_t** pStream) const {
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

Value DexParser::readEncodedValue(const uint8_t** pStream) const {
    const uint8_t* ptr = *pStream;
    uint8_t header = *(ptr++);
    
    // value_type is the low 5 bits, value_arg is the high 3 bits
    uint8_t value_type = header & 0x1f;
    uint8_t value_arg = header >> 5;
    
    // The size of the payload (number of bytes - 1)
    uint32_t size = value_arg + 1;
    
    Value result = Value::MakeNull();
    
    switch (value_type) {
        case 0x00: // VALUE_BYTE
        case 0x02: // VALUE_SHORT
        case 0x03: // VALUE_CHAR
        case 0x04: // VALUE_INT
        {
            int32_t val = 0;
            // Decode variable-length signed integer
            for (uint32_t i = 0; i < size; ++i) {
                val |= (static_cast<int32_t>(*(ptr++)) << (i * 8));
            }
            // Sign-extend if necessary
            if (size < 4 && (val & (1 << ((size * 8) - 1)))) {
                val |= -1 << (size * 8);
            }
            result = Value::MakeInt(val);
            break;
        }
        case 0x1d: // VALUE_ANNOTATION (Not implemented yet, skip it)
        case 0x1c: // VALUE_ARRAY
        case 0x17: // VALUE_STRING
        case 0x18: // VALUE_TYPE
        case 0x19: // VALUE_FIELD
        case 0x1a: // VALUE_METHOD
        case 0x1b: // VALUE_ENUM
        {
            // Just skip it for now
            ptr += size;
            break;
        }
        case 0x1f: // VALUE_BOOLEAN
        {
            // size is exactly the value (0 or 1)
            result = Value::MakeInt(value_arg); // Booleans are ints in our VM
            break;
        }
        case 0x1e: // VALUE_NULL
        {
            result = Value::MakeNull();
            break;
        }
        default:
            // Skip unknown types
            ptr += size;
            break;
    }
    
    *pStream = ptr;
    return result;
}

Value DexParser::getStaticFieldValue(uint32_t fieldIdx) const {
    if (fieldIdx >= m_fieldIdsSize || !m_classDefs || !m_dexBufferStart) {
        return Value::MakeNull();
    }
    
    const field_id_item& field = m_fieldIds[fieldIdx];
    uint16_t classIdx = field.class_idx;
    
    std::string className = m_strings[m_typeIds[classIdx].descriptor_idx];
    std::string fieldName = m_strings[field.name_idx];
    
    // Find class_def_item
    for (uint32_t i = 0; i < m_classDefsSize; ++i) {
        const class_def_item& classDef = m_classDefs[i];
        if (classDef.class_idx == classIdx) {
            
            if (classDef.class_data_off == 0) return Value::MakeNull();
            
            const uint8_t* ptr = m_dexBufferStart + classDef.class_data_off;
            uint32_t staticFieldsSize = readUnsignedLeb128(&ptr);
            
            // We don't care about the rest of the class_data_item counts
            readUnsignedLeb128(&ptr); // instanceFieldsSize
            readUnsignedLeb128(&ptr); // directMethodsSize
            readUnsignedLeb128(&ptr); // virtualMethodsSize
            
            // Find positional index in static_fields
            uint32_t positionalIndex = 0;
            uint32_t currentFieldIdx = 0;
            bool found = false;
            
            for (uint32_t f = 0; f < staticFieldsSize; ++f) {
                currentFieldIdx += readUnsignedLeb128(&ptr); // field_idx_diff
                readUnsignedLeb128(&ptr); // access_flags
                
                if (currentFieldIdx == fieldIdx) {
                    positionalIndex = f;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                Logger::w("DexParser", "Field " + className + "->" + fieldName + " is not static or not found in class_data_item");
                return Value::MakeNull();
            }
            
            // Now resolve the value from static_values_off
            if (classDef.static_values_off == 0) {
                Logger::w("DexParser", "Field " + className + "->" + fieldName + " has no static_values_off (defaulting to 0/null)");
                return Value::MakeNull();
            }
            
            const uint8_t* valPtr = m_dexBufferStart + classDef.static_values_off;
            uint32_t encodedArraySize = readUnsignedLeb128(&valPtr);
            
            if (positionalIndex >= encodedArraySize) {
                Logger::d("DexParser", "Field " + className + "->" + fieldName + " positional index " + std::to_string(positionalIndex) + " >= encoded_array size " + std::to_string(encodedArraySize) + " (defaulting to 0/null)");
                return Value::MakeNull(); // Implicit default value (0 or null)
            }
            
            // Fast-forward to the target value
            for (uint32_t e = 0; e < positionalIndex; ++e) {
                readEncodedValue(&valPtr);
            }
            
            Value val = readEncodedValue(&valPtr);
            Logger::d("DexParser", "Successfully read static value for " + className + "->" + fieldName);
            return val;
        }
    }
    
    Logger::w("DexParser", "Class " + className + " not found in this DEX (cross-DEX field access?) Cannot resolve " + fieldName);
    return Value::MakeNull();
}

Value DexParser::getStaticFieldValueByName(const std::string& className, const std::string& fieldName) const {
    if (!m_classDefs || !m_dexBufferStart) return Value::MakeUninitialized();
    
    // Find class_def_item by name
    for (uint32_t i = 0; i < m_classDefsSize; ++i) {
        const class_def_item& classDef = m_classDefs[i];
        std::string currentClassName = m_strings[m_typeIds[classDef.class_idx].descriptor_idx];
        
        if (currentClassName == className) {
            
            if (classDef.class_data_off == 0) return Value::MakeNull();
            
            const uint8_t* ptr = m_dexBufferStart + classDef.class_data_off;
            uint32_t staticFieldsSize = readUnsignedLeb128(&ptr);
            
            // Skip instance fields and methods
            readUnsignedLeb128(&ptr);
            readUnsignedLeb128(&ptr);
            readUnsignedLeb128(&ptr);
            
            uint32_t positionalIndex = 0;
            uint32_t currentFieldIdx = 0;
            bool found = false;
            
            for (uint32_t f = 0; f < staticFieldsSize; ++f) {
                currentFieldIdx += readUnsignedLeb128(&ptr); // field_idx_diff
                readUnsignedLeb128(&ptr); // access_flags
                
                const field_id_item& field = m_fieldIds[currentFieldIdx];
                std::string currentFieldName = m_strings[field.name_idx];
                
                if (currentFieldName == fieldName) {
                    positionalIndex = f;
                    found = true;
                    break;
                }
            }
            
            if (!found) return Value::MakeNull();
            
            // Resolve from static_values_off
            if (classDef.static_values_off == 0) return Value::MakeNull();
            
            const uint8_t* valPtr = m_dexBufferStart + classDef.static_values_off;
            uint32_t encodedArraySize = readUnsignedLeb128(&valPtr);
            
            if (positionalIndex >= encodedArraySize) {
                return Value::MakeNull(); // Implicit default
            }
            
            for (uint32_t e = 0; e < positionalIndex; ++e) {
                readEncodedValue(&valPtr);
            }
            
            return readEncodedValue(&valPtr);
        }
    }
    
    // Return UNINITIALIZED so MultiDexManager knows to keep looking
    return Value::MakeUninitialized();
}

std::string DexParser::getClassNameFromFieldIdx(uint32_t fieldIdx) const {
    if (fieldIdx >= m_fieldIdsSize) return "";
    const field_id_item& field = m_fieldIds[fieldIdx];
    return m_strings[m_typeIds[field.class_idx].descriptor_idx];
}

std::string DexParser::getFieldNameFromFieldIdx(uint32_t fieldIdx) const {
    if (fieldIdx >= m_fieldIdsSize) return "";
    const field_id_item& field = m_fieldIds[fieldIdx];
    return m_strings[field.name_idx];
}
