#include <iostream>
#include <string>
#include "apk_extractor/apkextractor.h"
#include "AxmlPraserer/AxmlParser.h"
#include "dex/DexParser.h"
#include "dex/MultiDexManager.h"
#include "utils/Logger.h"
#include "dex/ResourceManager.h"
#include "interpreter/Interpreter.h"
#include "interpreter/StubRegistry.h"
#include "ui/WindowManager.h"
#include <windows.h>
#include <commdlg.h>
#include <functional>

std::string getApkPathWithDialog() {
    char filename[MAX_PATH];
    ZeroMemory(filename, sizeof(filename));
    
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "APK Files\0*.apk\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select an APK to run with Windroid";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

std::string resolveMainActivity(const AxmlNode* root) {
    if (!root || root->tag != "manifest") return "";
    
    std::string packageName = "";
    for (const auto& attr : root->attributes) {
        if (attr.name == "package") {
            packageName = attr.rawValue;
            break;
        }
    }
    
    if (packageName.empty()) return "";

    std::string mainActivityName = "";

    std::function<void(const AxmlNode*)> searchNode = [&](const AxmlNode* node) {
        if (node->tag == "activity") {
            bool isMain = false;
            for (const auto& child : node->children) {
                if (child->tag == "intent-filter") {
                    for (const auto& intentChild : child->children) {
                        if (intentChild->tag == "action") {
                            for (const auto& attr : intentChild->attributes) {
                                if ((attr.name == "name" || attr.name == "android:name") && 
                                    attr.rawValue == "android.intent.action.MAIN") {
                                    isMain = true;
                                }
                            }
                        }
                    }
                }
            }
            if (isMain) {
                for (const auto& attr : node->attributes) {
                    if (attr.name == "name" || attr.name == "android:name") {
                        mainActivityName = attr.rawValue;
                        break;
                    }
                }
            }
        }
        for (const auto& child : node->children) {
            searchNode(child.get());
        }
    };
    
    searchNode(root);

    if (mainActivityName.empty()) return "";

    if (mainActivityName[0] == '.') {
        mainActivityName = packageName + mainActivityName;
    } else if (mainActivityName.find('.') == std::string::npos) {
        mainActivityName = packageName + "." + mainActivityName;
    }

    for (char& c : mainActivityName) {
        if (c == '.') c = '/';
    }
    
    return "L" + mainActivityName + ";";
}

int main(int argc, char* argv[]) {
    Logger::i("Main", "Windroid Runtime - APK Inspector Started");

    std::string apkPath = "";
    
    if (argc > 1) {
        apkPath = argv[1];
    } else {
        apkPath = getApkPathWithDialog();
        if (apkPath.empty()) {
            Logger::e("Main", "No APK selected. Exiting.");
            return 1;
        }
    }

    ApkExtractor extractor;
    if (!extractor.OpenApk(apkPath)) {
        Logger::e("Main", "Failed to open APK: " + apkPath);
        return 1;
    }

    Logger::i("Main", "Successfully opened APK.");
    
    std::vector<uint8_t> axmlBuffer;
    std::string mainActivityClass = "";
    if (extractor.ExtractEntryToMemory("AndroidManifest.xml", axmlBuffer)) {
        AxmlParser axmlParser;
        axmlParser.parse(axmlBuffer);
        
        mainActivityClass = resolveMainActivity(axmlParser.getRootNode());
        if (!mainActivityClass.empty()) {
            Logger::i("Main", "Resolved Main Activity: " + mainActivityClass);
        } else {
            Logger::w("Main", "Failed to resolve Main Activity from AndroidManifest.xml");
        }
    } else {
        Logger::e("Main", "Could not find AndroidManifest.xml in the APK!");
    }
    
    if (mainActivityClass.empty()) {
        Logger::w("Main", "Falling back to hardcoded MainActivity...");
        mainActivityClass = "Lcom/pranshu/test1/MainActivity;";
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

    // --- Phase 5: Resource Management ---
    ResourceManager resManager(&extractor);
    if (!resManager.init()) {
        Logger::e("Main", "Failed to initialize ResourceManager!");
    }

    // ---------------------------------------------------------
    // PHASE 2.5+: DYNAMIC BYTECODE EXTRACTION & INTERPRETATION
    // ---------------------------------------------------------
    Logger::i("Main", "Initializing StubRegistry...");
    StubRegistry::init(&resManager, &multiDexManager);
    
    Logger::i("Main", "Testing Interpreter Skeleton on real extracted bytecode...");
    Interpreter vm;
    
    // We can now ask the MultiDexManager for the method bytecode!
    auto [realBytecodeResult, currentDex] = multiDexManager.getMethodBytecode(mainActivityClass + "->onCreate(Landroid/os/Bundle;)V");
    
    if (!realBytecodeResult.bytecode.empty() && currentDex) {
        vm.executeMethod(realBytecodeResult.bytecode, currentDex, &multiDexManager, {}, realBytecodeResult.registers_size, realBytecodeResult.ins_size);
    } else {
        Logger::e("Main", "Failed to extract bytecode for " + mainActivityClass + ".onCreate!");
    }

    // Set up click routing to the interpreter
    WindowManager::setClickCallback([&](int controlId) {
        InterpreterObject* listener = StubRegistry::getClickListener(controlId);
        if (listener) {
            auto [clickBytecodeResult, clickDex] = multiDexManager.getMethodBytecode(listener->className + "->onClick(Landroid/view/View;)V");
            if (!clickBytecodeResult.bytecode.empty() && clickDex) {
                Logger::i("Main", "Executing click callback for " + listener->className);
                std::vector<Value> clickArgs;
                clickArgs.push_back(Value::MakeObject(listener)); // this
                clickArgs.push_back(Value::MakeNull());           // View parameter
                
                vm.executeMethod(clickBytecodeResult.bytecode, clickDex, &multiDexManager, clickArgs, clickBytecodeResult.registers_size, clickBytecodeResult.ins_size);
            } else {
                Logger::w("Main", "onClick method not found for class: " + listener->className);
            }
        }
    });

    // Block on Win32 Message Loop
    WindowManager::runMessageLoop();

    return 0;
}