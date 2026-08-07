#include <iostream>
#include <string>
#include "apk_extractor/apkextractor.h"
#include "AxmlPraserer/AxmlParser.h"
#include "dex/DexParser.h"
#include "dex/MultiDexManager.h"
#include "utils/Logger.h"
#include "interpreter/Interpreter.h"
#include "interpreter/StubRegistry.h"
#include "ui/WindowManager.h"

int main(int argc, char* argv[]) {
    Logger::i("Main", "Windroid Runtime - APK Inspector Started");

    std::string apkPath = "C:\\Users\\namde\\Documents\\Windroid\\testapk\\hellp.apk";
    
    if (argc > 1) {
        apkPath = argv[1];
    }

    ApkExtractor extractor;
    if (!extractor.OpenApk(apkPath)) {
        Logger::e("Main", "Failed to open APK: " + apkPath);
        return 1;
    }

    Logger::i("Main", "Successfully opened APK.");
    
    std::vector<uint8_t> axmlBuffer;
    if (extractor.ExtractEntryToMemory("AndroidManifest.xml", axmlBuffer)) {
        AxmlParser axmlParser;
        axmlParser.parse(axmlBuffer);
    } else {
        Logger::e("Main", "Could not find AndroidManifest.xml in the APK!");
    }

    // --- Phase 4: Multi-DEX Extraction ---
    MultiDexManager multiDexManager;
    
    int dexIndex = 1;
    while (true) {
        std::string dexName = (dexIndex == 1) ? "classes.dex" : "classes" + std::to_string(dexIndex) + ".dex";
        std::vector<uint8_t> dexBuffer;
        
        if (!extractor.ExtractEntryToMemory(dexName, dexBuffer)) {
            break; // No more DEX files
        }
        
        Logger::i("Main", "Loaded " + dexName);
        if (!multiDexManager.addDex(std::move(dexBuffer))) {
            Logger::e("Main", "Failed to parse " + dexName);
        }
        
        dexIndex++;
    }
    
    if (dexIndex == 1) {
        Logger::e("Main", "No classes.dex found in the APK!");
        return 1;
    }

    // --- Phase 4: Window Manager Init ---
    if (!WindowManager::init()) {
        return 1;
    }

    // ---------------------------------------------------------
    // PHASE 2.5+: DYNAMIC BYTECODE EXTRACTION & INTERPRETATION
    // ---------------------------------------------------------
    Logger::i("Main", "Initializing StubRegistry...");
    StubRegistry::init();
    
    Logger::i("Main", "Testing Interpreter Skeleton on real extracted bytecode...");
    Interpreter vm;
    
    // We can now ask the MultiDexManager for the method bytecode!
    auto [realBytecode, currentDex] = multiDexManager.getMethodBytecode("Lcom/pranshu/test1/MainActivity;", "onCreate");
    
    if (!realBytecode.empty() && currentDex) {
        vm.executeMethod(realBytecode, currentDex, &multiDexManager);
    } else {
        Logger::e("Main", "Failed to extract bytecode for MainActivity.onCreate!");
    }

    // Block on Win32 Message Loop
    WindowManager::runMessageLoop();

    return 0;
}