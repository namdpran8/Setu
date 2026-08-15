#include <ziparchive/zip_archive.h>
#include <iostream>
#include <vector>
#include <string>
#include <android-base/logging.h>

int main(int argc, char** argv) {
    android::base::InitLogging(argv, android::base::StderrLogger);
    android::base::SetMinimumLogSeverity(android::base::VERBOSE);
    const char* apk_path = "C:/Users/namde/Documents/Setu/testapk/openclalc.apk";
    
    ZipArchiveHandle handle;
    int32_t err = OpenArchive(apk_path, &handle);
    if (err != 0) {
        std::cerr << "Failed to open archive " << apk_path << ": err=" << err << " (" << ErrorCodeString(err) << ")" << std::endl;
        return 1;
    }

    std::string target_file = "resources.arsc";
    ZipEntry64 entry;
    err = FindEntry(handle, target_file, &entry);
    if (err != 0) {
        std::cerr << "Failed to find entry " << target_file << ": " << ErrorCodeString(err) << std::endl;
        CloseArchive(handle);
        return 1;
    }

    std::vector<uint8_t> buffer(entry.uncompressed_length);
    err = ExtractToMemory(handle, &entry, buffer.data(), buffer.size());
    if (err != 0) {
        std::cerr << "Failed to extract " << target_file << ": " << ErrorCodeString(err) << std::endl;
        CloseArchive(handle);
        return 1;
    }

    std::cout << "Successfully extracted " << target_file << "!" << std::endl;
    std::cout << "Uncompressed size: " << entry.uncompressed_length << " bytes." << std::endl;
    if (buffer.size() > 4) {
        std::cout << "First 4 bytes: " << std::hex 
                  << (int)buffer[0] << " " 
                  << (int)buffer[1] << " " 
                  << (int)buffer[2] << " " 
                  << (int)buffer[3] << std::dec << std::endl;
    }

    CloseArchive(handle);
    return 0;
}
