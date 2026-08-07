#include <iostream>
#include <string>
#include "apk_extractor/apkextractor.h"
#include "AxmlPraserer/AxmlParser.h"
#include "dex/DexParser.h"
#include "utils/Logger.h"
#include "interpreter/Interpreter.h"

int main(int argc, char* argv[]) {
    Logger::i("Main", "Windroid Runtime - APK Inspector Started");

    std::string apkPath = "C:\\Users\\namde\\Documents\\Windroid\\testapk\\hellp.apk"; // Default path
    
    // Allow overriding the path via command line
    if (argc > 1) {
        apkPath = argv[1];
    }

    ApkExtractor extractor;

    if (!extractor.OpenApk(apkPath)) {
        Logger::e("Main", "Failed to open APK: " + apkPath);
        return 1;
    }

    Logger::i("Main", "Successfully opened APK.");
    
    auto entries = extractor.listEntries();
    
    Logger::d("Main", "Found " + std::to_string(entries.size()) + " files in APK");
    
    std::vector<uint8_t> axmlBuffer;
    if (extractor.ExtractEntryToMemory("AndroidManifest.xml", axmlBuffer)) {
        AxmlParser axmlParser;
        axmlParser.parse(axmlBuffer);
    }
    else {
        Logger::e("Main", "Could not find AndroidManifest.xml in the APK!");
    }

    // --- Phase 1.3: DEX Parsing ---
    std::vector<uint8_t> dexBuffer;
    DexParser dexParser; // Declare outside so it stays alive for the Interpreter!
    
    // TEMPORARY: MainActivity is physically inside classes3.dex due to multidex!
    if (extractor.ExtractEntryToMemory("classes3.dex", dexBuffer)) {
        dexParser.parse(dexBuffer);
    } else {
        Logger::e("Main", "Could not find classes3.dex in the APK!");
        return 1;
    }

    // ---------------------------------------------------------
    // PHASE 2.5: DYNAMIC BYTECODE EXTRACTION & INTERPRETATION
    // ---------------------------------------------------------
    Logger::i("Main", "Testing Interpreter Skeleton on real extracted bytecode...");
    Interpreter vm;
    
    std::vector<uint8_t> realBytecode = dexParser.getMethodBytecode("Lcom/pranshu/test1/MainActivity;", "onCreate");
    
    if (!realBytecode.empty()) {
        vm.executeMethod(realBytecode, &dexParser);
    } else {
        Logger::e("Main", "Failed to extract bytecode for MainActivity.onCreate!");
    }

    return 0;
}