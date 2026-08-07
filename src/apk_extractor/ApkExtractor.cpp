#include <iostream>
#include <string>
#include <cstring> // For memset

#include "apkextractor.h"
#include "miniz.h"

using namespace std;

// -----------------------------------------------------------------------------
// Constructor: Setup the initial state
// -----------------------------------------------------------------------------
ApkExtractor::ApkExtractor() : m_isApkOpen(false) {
    // Allocate the miniz archive struct on the heap.
    m_zipArchive = new mz_zip_archive();
    
    // miniz REQUIRES the struct to be completely zeroed out before we call any
    // initialization functions. If we don't do this, it might crash.
    memset(m_zipArchive, 0, sizeof(mz_zip_archive)); 
}

// -----------------------------------------------------------------------------
// Destructor: Clean up when the object goes out of scope
// -----------------------------------------------------------------------------
ApkExtractor::~ApkExtractor() {
    // Ensure the file handle is closed if it was left open
    closeApk();
    
    // Free the memory we allocated in the constructor
    delete m_zipArchive;
}

// -----------------------------------------------------------------------------
// Open an APK for reading
// -----------------------------------------------------------------------------
bool ApkExtractor::OpenApk(const std::string& apkPath) {
    // mz_zip_reader_init_file takes the archive struct, the file path, and flags (0 is default).
    // It opens the file and reads the ZIP central directory so we know what's inside.
    bool success = mz_zip_reader_init_file(m_zipArchive, apkPath.c_str(), 0); 
    
    if (success) {
        m_isApkOpen = true;
    }
    
    return success;
}

// -----------------------------------------------------------------------------
// Close the APK
// -----------------------------------------------------------------------------
void ApkExtractor::closeApk() {
    if (m_isApkOpen) {
        // mz_zip_reader_end frees the internal memory miniz used for the central directory
        // and closes the file handle on disk.
        mz_zip_reader_end(m_zipArchive);
        m_isApkOpen = false;
    }
}

// -----------------------------------------------------------------------------
// List all files inside the APK
// -----------------------------------------------------------------------------
std::vector<ApkEntry> ApkExtractor::listEntries() const {
    std::vector<ApkEntry> entries;
    
    if (!m_isApkOpen) {
        return entries; // Return empty list if no APK is open
    }
    
    // Get total number of files/directories inside the ZIP archive
    mz_uint numFiles = mz_zip_reader_get_num_files(m_zipArchive);
    
    for (mz_uint i = 0; i < numFiles; ++i) {
        mz_zip_archive_file_stat file_stat;
        
        // Grab the metadata (stat) for the file at index 'i'
        if (mz_zip_reader_file_stat(m_zipArchive, i, &file_stat)) {
            // We usually don't care about directory entries (like "META-INF/") when parsing APKs.
            // We only care about actual files (like "classes.dex").
            if (mz_zip_reader_is_file_a_directory(m_zipArchive, i)) {
                continue;
            }
            
            ApkEntry entry;
            entry.filename = file_stat.m_filename;
            entry.uncompressedSize = file_stat.m_uncomp_size;
            entry.compressedSize = file_stat.m_comp_size;
            
            // Method 0 means STORED (uncompressed). Usually, method 8 means DEFLATED (compressed).
            entry.isCompressed = (file_stat.m_method != 0);
            
            entries.push_back(entry);
        }
    }
    return entries;
}

// -----------------------------------------------------------------------------
// Extract a file directly into a memory buffer
// -----------------------------------------------------------------------------
bool ApkExtractor::ExtractEntryToMemory(const std::string& entryName, std::vector<uint8_t>& outBuffer) const {
    if (!m_isApkOpen) return false;

    size_t uncomp_size = 0;
    
    // This tells miniz to find the file by name, allocate memory for it using malloc(),
    // decompress it into that memory, and return a pointer to it.
    void* pBuffer = mz_zip_reader_extract_file_to_heap(m_zipArchive, entryName.c_str(), &uncomp_size, 0);
    
    if (!pBuffer) {
        return false; // File not found or extraction failed
    }
    
    // Cast the raw void* to a byte pointer
    uint8_t* bytePtr = static_cast<uint8_t*>(pBuffer);
    
    // Copy the raw bytes into our C++ std::vector. The vector manages its own memory safely.
    outBuffer.assign(bytePtr, bytePtr + uncomp_size);
    
    // CRITICAL: We must free the memory that miniz allocated, otherwise we create a memory leak!
    mz_free(pBuffer);
    
    return true;
}

// -----------------------------------------------------------------------------
// Extract a file to disk
// -----------------------------------------------------------------------------
bool ApkExtractor::ExtractEntry(const std::string& entryName, const std::string& outputPath) const {
    if (!m_isApkOpen) return false;

    // This is a helper function in miniz that extracts directly to a file on your hard drive.
    return mz_zip_reader_extract_file_to_file(m_zipArchive, entryName.c_str(), outputPath.c_str(), 0);
}

#include "../utils/Logger.h"

// -----------------------------------------------------------------------------
// Dummy test function
// -----------------------------------------------------------------------------
void ApkExtractor::extractor() {
    Logger::i("ApkExtractor", "This extractor class is working");
}