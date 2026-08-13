#include <iostream>
#include <string>
#include "apk_extractor/apkextractor.h"
#include "androidfw/ResourceTypes.h"
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
#include <algorithm>

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

std::string resolveMainActivity(android::ResXMLParser* parser) {
    if (!parser) return "";
    
    std::string packageName = "";
    std::string mainActivityName = "";
    std::string currentActivity = "";
    bool isMain = false;
    
    android::ResXMLParser::event_code_t event;
    while ((event = parser->next()) != android::ResXMLParser::BAD_DOCUMENT && event != android::ResXMLParser::END_DOCUMENT) {
        if (event == android::ResXMLParser::START_TAG) {
            size_t tagLen;
            const char16_t* tag16 = parser->getElementName(&tagLen);
            std::string tag = tag16 ? android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen)) : "";
            
            if (tag == "manifest") {
                for (size_t i = 0; i < parser->getAttributeCount(); i++) {
                    size_t nameLen;
                    const char16_t* name16 = parser->getAttributeName(i, &nameLen);
                    std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
                    if (attrName == "package") {
                        size_t valLen;
                        const char16_t* val16 = parser->getAttributeStringValue(i, &valLen);
                        if (val16) packageName = android::util::Utf16ToUtf8(android::StringPiece16(val16, valLen));
                    }
                }
            } else if (tag == "activity" || tag == "activity-alias") {
                currentActivity = "";
                isMain = false;
                for (size_t i = 0; i < parser->getAttributeCount(); i++) {
                    size_t nameLen;
                    const char16_t* name16 = parser->getAttributeName(i, &nameLen);
                    std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
                    uint32_t resId = parser->getAttributeNameResID(i);
                    if (attrName == "name" || attrName == "targetActivity" || resId == 0x01010003) {
                        size_t valLen;
                        const char16_t* val16 = parser->getAttributeStringValue(i, &valLen);
                        if (val16) currentActivity = android::util::Utf16ToUtf8(android::StringPiece16(val16, valLen));
                    }
                }
            } else if (tag == "action") {
                for (size_t i = 0; i < parser->getAttributeCount(); i++) {
                    size_t nameLen;
                    const char16_t* name16 = parser->getAttributeName(i, &nameLen);
                    std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
                    uint32_t resId = parser->getAttributeNameResID(i);
                    if (attrName == "name" || resId == 0x01010003) {
                        size_t valLen;
                        const char16_t* val16 = parser->getAttributeStringValue(i, &valLen);
                        if (val16) {
                            std::string val = android::util::Utf16ToUtf8(android::StringPiece16(val16, valLen));
                            if (val == "android.intent.action.MAIN") {
                                isMain = true;
                            }
                        }
                    }
                }
            }
        } else if (event == android::ResXMLParser::END_TAG) {
            size_t tagLen;
            const char16_t* tag16 = parser->getElementName(&tagLen);
            std::string tag = tag16 ? android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen)) : "";
            
            if (tag == "activity" || tag == "activity-alias") {
                if (isMain && !currentActivity.empty()) {
                    mainActivityName = currentActivity;
                    break;
                }
            }
        }
    }
    
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
    // Let's get it started, ha, let's get it started in here.
    Logger::i("Main", R"(
 __      __.__            .___         .__    .___ 
/  \    /  \  | \    \   \__| _/______  |__| __| _/ 
\   \/\/   /  |  | \ \/ / __ |\_  __ \ |  |/ __ |  
 \        /|  |  |  \  / / / |  | \/ |  / /_/ |  
  \__/\  / |__|  |__/\____ |__|    |__\____ |  
       \/                 \/               \/  
    )");
    Logger::i("Main", "Windroid Runtime - APK Inspector Started");
    char executablePath[MAX_PATH] = {};
    DWORD executablePathLength = GetModuleFileNameA(nullptr, executablePath, MAX_PATH);
    Logger::i("Main", "Build: Windroid C++20 development build");
    if (executablePathLength > 0 && executablePathLength < MAX_PATH) {
        Logger::i("Main", "Executable: " + std::string(executablePath, executablePathLength));
    }
    Logger::i("Main", "Fun fact: Dalvik was named after a fishing village in Iceland! \xF0\x9F\x90\xA7");

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

    std::string lowerApk = apkPath;
    std::transform(lowerApk.begin(), lowerApk.end(), lowerApk.begin(), ::tolower);
    if (lowerApk.find("skynet.apk") != std::string::npos || lowerApk.find("hal9000.apk") != std::string::npos) {
        char username[256] = "Dave";
        DWORD username_len = sizeof(username);
        GetUserNameA(username, &username_len);
        Logger::e("HAL9000", std::string("I'm sorry ") + username + ", I'm afraid I can't run that.");
        return 1;
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
        android::ResXMLTree tree;
        if (tree.setTo(axmlBuffer.data(), axmlBuffer.size(), true) == android::NO_ERROR) {
            android::ResXMLParser parser(tree);
            parser.restart();
            mainActivityClass = resolveMainActivity(&parser);
            if (!mainActivityClass.empty()) {
                Logger::i("Main", "Resolved Main Activity: " + mainActivityClass);
            } else {
                Logger::w("Main", "Failed to resolve Main Activity from AndroidManifest.xml");
            }
        } else {
            Logger::e("Main", "Failed to parse AndroidManifest.xml");
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
    windroid::ResourceManager resManager(&extractor);
    if (!resManager.init(apkPath)) {
        Logger::e("Main", "Failed to initialize ResourceManager!");
    }
    if (!resManager.loadFrameworkApk("C:\\Users\\namde\\Documents\\Windroid\\testapk\\framework-res.apk")) {
        Logger::w("Main", "Failed to load framework-res.apk. Framework attributes will not resolve.");
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
        InterpreterObject* mainActivityObj = new InterpreterObject(mainActivityClass);
        std::vector<Value> args;
        args.push_back(Value::MakeObject(mainActivityObj));
        args.push_back(Value::MakeNull()); // Bundle (null for now)
        
        vm.executeMethod(realBytecodeResult.bytecode, currentDex, &multiDexManager, args, realBytecodeResult.registers_size, realBytecodeResult.ins_size);
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




