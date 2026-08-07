#include <iostream>
#include "apk_extractor/apkextractor.h"
#include "AxmlPraserer/AxmlParser.h"

int main(int argc, char* argv[]) {
    std::cout << "Windroid Runtime - APK Inspector" << std::endl;

    std::string apkPath = "C:\\Users\\namde\\Documents\\Windroid\\testapk\\hellp.apk"; // Default path
    
    // Allow overriding the path via command line
    if (argc > 1) {
        apkPath = argv[1];
    }

    ApkExtractor extractor;

    if (!extractor.OpenApk(apkPath)) {
        std::cerr << "Failed to open APK: " << apkPath << std::endl;
        return 1;
    }

    std::cout << "Successfully opened APK. Listing contents:" << std::endl;
    
    auto entries = extractor.listEntries();
    
    std::cout << "Found " << entries.size() << " files:" << std::endl;
    
    std::vector<uint8_t> axmlBuffer;
    if (extractor.ExtractEntryToMemory("AndroidManifest.xml", axmlBuffer)) {
        AxmlParser axmlParser;
        axmlParser.parse(axmlBuffer);
    }
    else {
        std::cerr << "Could not find AndroidManifest.xml in the APK!" << std::endl;
    }

    return 0;
}