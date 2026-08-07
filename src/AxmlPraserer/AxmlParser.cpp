#include "AxmlParser.h"
#include <iostream>

using namespace std;

AxmlParser::AxmlParser() {
    // Constructor
}

AxmlParser::~AxmlParser() {
    // Destructor
}

// -----------------------------------------------------------------------------
// Main parse function. This takes the raw binary buffer we extracted from the ZIP.
// -----------------------------------------------------------------------------
bool AxmlParser::parse(const std::vector<uint8_t>& axmlBuffer) {
    if (axmlBuffer.empty()) {
        cerr << "AXML buffer is empty!" << endl;
        return false;
    }

    // Step 1: Read the very first chunk in the file to make sure it's valid AXML
    // We cast our raw byte array directly into our ResChunk_header struct!
    const ResChunk_header* header = reinterpret_cast<const ResChunk_header*>(axmlBuffer.data());

    // 0x0003 is the magic number for RES_XML_TYPE (AXML file type)
    if (header->type != 0x0003) {
        cerr << "Invalid AXML file (bad magic number)!" << endl;
        return false;
    }

    cout << "Valid AXML file detected. Total size: " << header->size << " bytes." << endl;

    // TODO: In the next step, we will loop through the chunks that come after this header
    // and parse the String Pool!
    // The string pool chunk starts immediately after the main 8-byte AXML header
    const uint8_t* stringPoolStart = axmlBuffer.data() + sizeof(ResChunk_header);

    if (!parseStringPool(stringPoolStart)) {
        return false;
    }

    // --- PHASE 1.2 MILESTONE: Read the XML Tags! ---
    // The string pool chunk size tells us exactly where the rest of the chunks begin
    const ResChunk_header* stringPoolHeader = reinterpret_cast<const ResChunk_header*>(stringPoolStart);
    const uint8_t* currentPtr = stringPoolStart + stringPoolHeader->size;
    const uint8_t* endPtr = axmlBuffer.data() + axmlBuffer.size();

    cout << "\n--- XML TREE DUMP ---" << endl;

    // Loop through all remaining chunks until the end of the file
    while (currentPtr < endPtr) {
        const ResChunk_header* chunk = reinterpret_cast<const ResChunk_header*>(currentPtr);
        
        // Safety check to prevent infinite loops on corrupted files
        if (chunk->size == 0) break;

        // 0x0102 is the magic number for RES_XML_START_ELEMENT_TYPE (e.g. <manifest>, <activity>)
        if (chunk->type == 0x0102) {
            
            // The node header is right at the start of this chunk
            const ResXMLTree_node* node = reinterpret_cast<const ResXMLTree_node*>(currentPtr);
            
            // The attribute extension struct follows immediately after the node header
            const ResXMLTree_attrExt* attrExt = reinterpret_cast<const ResXMLTree_attrExt*>(currentPtr + sizeof(ResXMLTree_node));
            
            // Look up the tag name from our string pool using the index (0xFFFFFFFF means it has no name)
            string tagName = (attrExt->name != 0xFFFFFFFF) ? m_stringPool[attrExt->name] : "UNKNOWN";
            
            cout << "Found Tag: <" << tagName << ">" << endl;

            // Now loop through its attributes!
            // The attributes array starts at an offset specified in attrExt
            const uint8_t* attrPtr = currentPtr + sizeof(ResXMLTree_node) + attrExt->attributeStart;
            
            for (int i = 0; i < attrExt->attributeCount; ++i) {
                const ResXMLTree_attribute* attr = reinterpret_cast<const ResXMLTree_attribute*>(attrPtr);
                
                string attrName = (attr->name != 0xFFFFFFFF) ? m_stringPool[attr->name] : "UNKNOWN";
                
                // If it's a normal string attribute, rawValue is a valid index into the string pool
                if (attr->rawValue != 0xFFFFFFFF) {
                    cout << "    " << attrName << "=\"" << m_stringPool[attr->rawValue] << "\"" << endl;
                } else {
                    // It's a typed value (like an integer, a boolean, or a resource reference like @string/app_name)
                    // For now, we'll just print its raw hex data.
                    cout << "    " << attrName << "=(TypedData: 0x" << std::hex << attr->typedValue_data << std::dec << ")" << endl;
                }
                
                // Move to the next attribute struct
                attrPtr += attrExt->attributeSize;
            }
        }
        
        // Move our pointer forward by exactly the size of this entire chunk to find the next one
        currentPtr += chunk->size;
    }


    return true;
}

// -----------------------------------------------------------------------------
// Helper to parse the string pool chunk
// -----------------------------------------------------------------------------
bool AxmlParser::parseStringPool(const uint8_t* chunkStart) {
    // Cast the raw bytes into our String Pool struct
    const ResStringPool_header* header = reinterpret_cast<const ResStringPool_header*>(chunkStart);

    // 0x0001 is the magic number for RES_STRING_POOL_TYPE
    if (header->header.type != 0x0001) {
        cerr << "Expected String Pool chunk, but got type: " << header->header.type << endl;
        return false;
    }

    cout << "Found String Pool! It contains " << header->stringCount << " strings." << endl;

    // Is the UTF-8 flag set? (1 << 8 is 256)
    bool isUTF8 = (header->flags & 256) != 0;
    cout << "String encoding is: " << (isUTF8 ? "UTF-8" : "UTF-16") << endl;

    // 1. Find where the offsets array starts
    const uint32_t* stringOffsets = reinterpret_cast<const uint32_t*>(chunkStart + header->header.headerSize);
    
    // 2. Find where the actual string byte data starts
    const uint8_t* stringData = chunkStart + header->stringsStart;

    cout << "--- STRING POOL DUMP ---" << endl;

    // 3. Loop through every string index
    for (uint32_t i = 0; i < header->stringCount; ++i) {
        
        // Jump to the offset for this specific string
        const uint16_t* strPtr = reinterpret_cast<const uint16_t*>(stringData + stringOffsets[i]);
        
        // Read the length (Android strings can do a weird 2-word length if they are huge)
        uint32_t length = *strPtr++;
        if ((length & 0x8000) != 0) {
            length = ((length & 0x7FFF) << 16) | (*strPtr++);
        }

        // Convert the 16-bit characters into a normal std::string (Manual UTF-16 to UTF-8)
        const char16_t* chars = reinterpret_cast<const char16_t*>(strPtr);
        std::string parsedString;
        
        for (uint32_t c = 0; c < length; ++c) {
            char16_t ch = chars[c];
            // For standard AndroidManifest ASCII characters (like 'android', 'name')
            if (ch < 0x80) {
                parsedString.push_back(static_cast<char>(ch));
            } else {
                // If you encounter foreign languages or emojis, you'd add multi-byte 
                // UTF-8 encoding here. For our Phase 1 milestone, simply stripping 
                // to ASCII is perfectly fine!
                parsedString.push_back('?'); 
            }
        }

        // Save it in our class vector!
        m_stringPool.push_back(parsedString);

        // Print the first 15 strings just to prove it works!
        if (i < 15) {
            cout << "String [" << i << "]: " << parsedString << endl;
        }
    }

    return true;
}