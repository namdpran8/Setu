#include "ArscParser.h"
#include "../utils/Logger.h"

ArscParser::ArscParser() {
}

ArscParser::~ArscParser() {
}

void ArscParser::parseStringPool(const uint8_t* ptr, std::vector<std::string>& outStrings) {
    const ResStringPool_header* header = reinterpret_cast<const ResStringPool_header*>(ptr);
    
    bool isUTF8 = (header->flags & 256) != 0;
    const uint32_t* stringOffsets = reinterpret_cast<const uint32_t*>(ptr + header->header.headerSize);
    const uint8_t* stringData = ptr + header->stringsStart;
    
    for (uint32_t i = 0; i < header->stringCount; ++i) {
        if (isUTF8) {
            const uint8_t* strPtr = stringData + stringOffsets[i];
            
            // Skip length bytes for UTF-8
            if (*strPtr & 0x80) strPtr += 2;
            else strPtr += 1;
            
            if (*strPtr & 0x80) strPtr += 2;
            else strPtr += 1;
            
            std::string parsedString;
            while (*strPtr != '\0') {
                parsedString += (char)*strPtr++;
            }
            outStrings.push_back(parsedString);
        } else {
            const uint16_t* strPtr = reinterpret_cast<const uint16_t*>(stringData + stringOffsets[i]);
            
            uint32_t length = *strPtr++;
            if ((length & 0x8000) != 0) {
                length = ((length & 0x7FFF) << 16) | (*strPtr++);
            }
            
            const char16_t* chars = reinterpret_cast<const char16_t*>(strPtr);
            std::string parsedString;
            
            for (uint32_t c = 0; c < length; ++c) {
                char16_t ch = chars[c];
                if (ch < 0x80) {
                    parsedString += (char)ch;
                } else {
                    parsedString += '?';
                }
            }
            outStrings.push_back(parsedString);
        }
    }
}

bool ArscParser::parse(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(ResTable_header)) return false;
    
    const uint8_t* ptr = buffer.data();
    const ResTable_header* header = reinterpret_cast<const ResTable_header*>(ptr);
    
    if (header->header.type != 0x0002) {
        Logger::e("ArscParser", "Invalid resources.arsc type");
        return false;
    }
    
    ptr += header->header.headerSize;
    uint32_t bytesRemaining = header->header.size - header->header.headerSize;
    
    while (bytesRemaining > sizeof(ResChunk_header)) {
        const ResChunk_header* chunkHeader = reinterpret_cast<const ResChunk_header*>(ptr);
        
        if (chunkHeader->type == 0x0001) { // String Pool
            parseStringPool(ptr, m_globalStrings);
            Logger::d("ArscParser", "Parsed global string pool: " + std::to_string(m_globalStrings.size()) + " strings");
        } else if (chunkHeader->type == 0x0200) { // Package
            parsePackage(ptr);
        }
        
        ptr += chunkHeader->size;
        bytesRemaining -= chunkHeader->size;
    }
    
    return true;
}

void ArscParser::parsePackage(const uint8_t* ptr) {
    const ResTable_package* pkgHeader = reinterpret_cast<const ResTable_package*>(ptr);
    uint32_t packageId = pkgHeader->id;
    m_packageId = (uint8_t)packageId;
    
    Logger::d("ArscParser", "Parsing package ID: " + std::to_string(packageId));
    
    // Parse Type and Key String pools
    if (pkgHeader->typeStrings > 0) {
        parseStringPool(ptr + pkgHeader->typeStrings, m_typeStrings);
    }
    if (pkgHeader->keyStrings > 0) {
        parseStringPool(ptr + pkgHeader->keyStrings, m_keyStrings);
    }
    
    const uint8_t* chunkPtr = ptr + pkgHeader->header.headerSize;
    uint32_t bytesRemaining = pkgHeader->header.size - pkgHeader->header.headerSize;
    
    while (bytesRemaining > sizeof(ResChunk_header)) {
        const ResChunk_header* chunkHeader = reinterpret_cast<const ResChunk_header*>(chunkPtr);
        
        if (chunkHeader->type == 0x0201) { // Type Chunk
            const ResTable_type* typeHeader = reinterpret_cast<const ResTable_type*>(chunkPtr);
            uint32_t typeId = typeHeader->id;
            
            const uint8_t* indices = chunkPtr + typeHeader->header.headerSize;
            const uint8_t* entriesStart = chunkPtr + typeHeader->entriesStart;
            bool isSparse = (typeHeader->flags & 0x01) != 0;
            bool isOffset16 = (typeHeader->flags & 0x02) != 0;
            const uint8_t* chunkEnd = chunkPtr + chunkHeader->size;
            
            for (uint32_t i = 0; i < typeHeader->entryCount; ++i) {
                uint32_t entryIndex;
                uint32_t offset;
                
                if (isSparse) {
                    const ResTable_sparseTypeEntry* sparseIndices = reinterpret_cast<const ResTable_sparseTypeEntry*>(indices);
                    entryIndex = sparseIndices[i].idx;
                    offset = sparseIndices[i].offset;
                    if (offset != 0xFFFF) offset *= 4;
                } else {
                    entryIndex = i;
                    if (isOffset16) {
                        const uint16_t* indices16 = reinterpret_cast<const uint16_t*>(indices);
                        offset = indices16[i];
                        if (offset != 0xFFFF) offset *= 4;
                    } else {
                        const uint32_t* indices32 = reinterpret_cast<const uint32_t*>(indices);
                        offset = indices32[i];
                    }
                }
                
                if (offset != 0xFFFFFFFF && offset != 0xFFFF) {
                    const uint8_t* entryPtr = entriesStart + offset;
                    
                    // Simple bounds check
                    if (entryPtr + sizeof(ResTable_entry) <= chunkEnd) {
                        const ResTable_entry* entryHeader = reinterpret_cast<const ResTable_entry*>(entryPtr);
                        
                        // Simple entry (no FLAG_COMPLEX)
                        if ((entryHeader->flags & 0x0001) == 0) {
                            if (entryPtr + entryHeader->size + sizeof(Res_value) <= chunkEnd) {
                                const Res_value* value = reinterpret_cast<const Res_value*>(entryPtr + entryHeader->size);
                                if (value->dataType == 0x03) {
                                    uint32_t resId = (packageId << 24) | (typeId << 16) | entryIndex;
                                    m_resourceStringPoolIndices[resId] = value->data;
                                }
                            }
                        } else { // FLAG_COMPLEX
                            if (entryPtr + sizeof(ResTable_map_entry) <= chunkEnd) {
                                const ResTable_map_entry* mapEntry = reinterpret_cast<const ResTable_map_entry*>(entryHeader);
                                uint32_t resId = (packageId << 24) | (typeId << 16) | entryIndex;
                                
                                Bag bag;
                                bag.parentResId = mapEntry->parent;
                                
                                const ResTable_map* mapStart = reinterpret_cast<const ResTable_map*>(entryPtr + mapEntry->size);
                                if (reinterpret_cast<const uint8_t*>(mapStart + mapEntry->count) <= chunkEnd) {
                                    bag.maps.assign(mapStart, mapStart + mapEntry->count);
                                    m_bags[resId] = bag;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        chunkPtr += chunkHeader->size;
        bytesRemaining -= chunkHeader->size;
    }
}

std::string ArscParser::resolveStringValue(uint32_t resId) const {
    auto it = m_resourceStringPoolIndices.find(resId);
    if (it != m_resourceStringPoolIndices.end()) {
        uint32_t stringIndex = it->second;
        if (stringIndex < m_globalStrings.size()) {
            return m_globalStrings[stringIndex];
        }
    }
    return "";
}

const ArscParser::Bag* ArscParser::getBag(uint32_t resId) const {
    auto it = m_bags.find(resId);
    if (it != m_bags.end()) {
        return &it->second;
    }
    return nullptr;
}
