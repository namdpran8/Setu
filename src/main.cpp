#include "ui/LayoutInflater.h"
#include "ui/Theme.h"
/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
#include "animation/PropertyRegistry.h"
#include <windows.h>
#include <commdlg.h>
#include <functional>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>

/*
    Hey, you.

    Yes, you reading the raw main.cpp source code .

    First of all, why?

    Second, welcome.

    This project was supposed to be a small experiment.
    Somehow it turned into whatever this is.

    If you're still reading, congratulations.
    You have officially spent more time reading this comment
    than I spent planning the architecture.
*/

struct LaunchArgs {
    std::string package;
    std::string logLevel = "info";
    std::string frameworkApk = "";
};

void crashExit(int code, const std::string& package, const std::string& message) {
    if (!package.empty()) {
        const char* localAppData = getenv("LOCALAPPDATA");
        if (localAppData) {
            std::string cacheDir = std::string(localAppData) + "\\Setu\\apps\\" + package;
            std::error_code ec;
            std::filesystem::create_directories(cacheDir, ec);
            std::string crashLogPath = cacheDir + "\\crash.log";
            
            std::ofstream ofs(crashLogPath, std::ios::out | std::ios::trunc);
            if (ofs.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                ofs << "Timestamp: " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "\n";
                ofs << "Exit Code: " << code << "\n";
                ofs << "Message: " << message << "\n";
            }
        }
    }
    exit(code);
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
// If you\'re reading this, you\'re probably trying to understand what this does.
// Good luck. The comment above was written by AI because I didn\'t want to
// explain it myself. I hope it was correct.
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
   _____      __       
  / ___/___  / /___  __
  \__ \/ _ \/ __/ / / /
 ___/ /  __/ /_/ /_/ / 
/____/\___/\__/\__,_/  
    )");
    Logger::i("Main", "Setu Runtime - APK Inspector Started");
    char executablePath[MAX_PATH] = {};
    DWORD executablePathLength = GetModuleFileNameA(nullptr, executablePath, MAX_PATH);
    Logger::i("Main", "Build: Setu C++20 development build");
    if (executablePathLength > 0 && executablePathLength < MAX_PATH) {
        Logger::i("Main", "Executable: " + std::string(executablePath, executablePathLength));
    }
    Logger::i("Main", "Fun fact: Dalvik was named after a fishing village in Iceland! \xF0\x9F\x90\xA7");

    LaunchArgs launchArgs;
    std::string apkPath;
    std::string appPath;
    std::string iconPath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.find("--package=") == 0) {
            launchArgs.package = arg.substr(10);
        } else if (arg.find("--app-path=") == 0) {
            appPath = arg.substr(11);
        } else if (arg.find("--icon-path=") == 0) {
            iconPath = arg.substr(12);
        } else if (arg.find("--log-level=") == 0) {
            launchArgs.logLevel = arg.substr(12);
        } else if (arg.find("--framework=") == 0) {
            launchArgs.frameworkApk = arg.substr(12);
        } else if (arg[0] != '-') {
            // Positional argument, assume APK path
            apkPath = arg;
        }
    }

    if (apkPath.empty() && appPath.empty()) {
        if (launchArgs.package.empty()) {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = { 0 };
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = NULL;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = "APK Files\0*.apk\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameA(&ofn) == TRUE) {
                apkPath = ofn.lpstrFile;
            } else {
                return 0;
            }
        } else {
            const char* localAppData = getenv("LOCALAPPDATA");
            std::string cacheDir = std::string(localAppData ? localAppData : "") + "\\Setu\\apps\\" + launchArgs.package;
            apkPath = cacheDir + "\\base.apk";
        }
    }

    Logger::setConfiguredLevel(launchArgs.logLevel);

    std::string lowerApk = apkPath;
    std::transform(lowerApk.begin(), lowerApk.end(), lowerApk.begin(), ::tolower);
    if (lowerApk.find("skynet.apk") != std::string::npos || lowerApk.find("hal9000.apk") != std::string::npos) {
        char username[256] = "Dave";
        DWORD username_len = sizeof(username);
        GetUserNameA(username, &username_len);
        Logger::e("HAL9000", std::string("I'm sorry ") + username + ", I'm afraid I can't run that.");
        crashExit(99, launchArgs.package, "HAL9000 error");
    }

    bool isDirectory = !appPath.empty();
    std::string loadPath = isDirectory ? appPath : apkPath;
    
    setu::ResourceManager resManager;
    if (!resManager.init(loadPath, isDirectory)) {
        Logger::e("Main", "Failed to initialize ResourceManager for: " + loadPath);
        crashExit(2, launchArgs.package, "Failed to initialize ResourceManager");
    }
    
    Logger::i("Main", "Successfully initialized ResourceManager.");
    
    std::vector<uint8_t> axmlBuffer;
    std::string mainActivityClass = "";
    
    auto manifestAsset = resManager.getAssetManager()->OpenNonAsset("AndroidManifest.xml", android::Asset::ACCESS_BUFFER);
    if (manifestAsset) {
        axmlBuffer.assign(
            (const uint8_t*)manifestAsset->getBuffer(true),
            (const uint8_t*)manifestAsset->getBuffer(true) + manifestAsset->getLength());
            
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
        Logger::e("Main", "Could not find AndroidManifest.xml in the APK/Directory!");
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
        
        auto dexAsset = resManager.getAssetManager()->OpenNonAsset(dexName, android::Asset::ACCESS_BUFFER);
        if (!dexAsset) {
            break; // No more DEX files
        }
        
        std::vector<uint8_t> dexBuffer;
        dexBuffer.assign(
            (const uint8_t*)dexAsset->getBuffer(true),
            (const uint8_t*)dexAsset->getBuffer(true) + dexAsset->getLength());
        
        Logger::i("Main", "Loaded " + dexName);
        if (!multiDexManager.addDex(std::move(dexBuffer))) {
            Logger::e("Main", "Failed to parse " + dexName);
        }
        
        dexIndex++;
    }
    
    if (dexIndex == 1) {
        Logger::e("Main", "No classes.dex found in the APK/Directory!");
        crashExit(2, launchArgs.package, "No classes.dex found in the APK/Directory");
    }

    // --- Phase 4: Window Manager Init ---
    if (!WindowManager::init()) {
        crashExit(3, launchArgs.package, "Window/Render initialization failed");
    }

    if (!iconPath.empty()) {
        WindowManager::setWindowIcon(iconPath);
    }

    // --- Phase 5: Resource Management ---
    std::string frameworkApkPath = launchArgs.frameworkApk;
    
    if (frameworkApkPath.empty()) {
        std::vector<std::string> searchPaths = {
            "apkresources\\framework-res.apk",
            "framework-res.apk",
            "..\\apkresources\\framework-res.apk"
        };
        
        if (executablePathLength > 0 && executablePathLength < MAX_PATH) {
            std::filesystem::path exePath(std::string(executablePath, executablePathLength));
            searchPaths.insert(searchPaths.begin(), (exePath.parent_path() / "framework-res.apk").string());
            searchPaths.insert(searchPaths.begin(), (exePath.parent_path() / "apkresources" / "framework-res.apk").string());
        }

        for (const auto& path : searchPaths) {
            if (std::filesystem::exists(path)) {
                frameworkApkPath = path;
                break;
            }
        }
        
        if (frameworkApkPath.empty()) {
            frameworkApkPath = "apkresources\\framework-res.apk"; // Fallback
        }
    }

    Logger::i("Main", "Attempting to load framework APK from: " + frameworkApkPath);
    if (!resManager.loadFrameworkApk(frameworkApkPath)) {
        Logger::w("Main", "Failed to load framework-res.apk. Framework attributes will not resolve.");
		//In future TODO: We might want to make this a fatal error if the framework-res.apk is not found, as many apps rely on it. For now, we will just log a warning.
       // crashExit(2, launchArgs.package, "framework-res.apk load failure");
    } else {
        Logger::i("Main", "Framework APK loaded successfully from: " + frameworkApkPath);
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
        
        Value res = vm.executeMethod(realBytecodeResult.bytecode, currentDex, &multiDexManager, args, realBytecodeResult.registers_size, realBytecodeResult.ins_size);
        if (res.type == ValueType::OBJECT && res.obj && static_cast<InterpreterObject*>(res.obj)->className.find("Exception") != std::string::npos) {
            crashExit(1, launchArgs.package, "Uncaught interpreter exception: " + static_cast<InterpreterObject*>(res.obj)->className + " in " + mainActivityClass + "->onCreate()");
        }
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
                
                Value clickRes = vm.executeMethod(clickBytecodeResult.bytecode, clickDex, &multiDexManager, clickArgs, clickBytecodeResult.registers_size, clickBytecodeResult.ins_size);
                if (clickRes.type == ValueType::OBJECT && clickRes.obj && static_cast<InterpreterObject*>(clickRes.obj)->className.find("Exception") != std::string::npos) {
                    crashExit(1, launchArgs.package, "Uncaught interpreter exception: " + static_cast<InterpreterObject*>(clickRes.obj)->className + " in " + listener->className + "->onClick()");
                }
            } else {
                Logger::w("Main", "onClick method not found for class: " + listener->className);
            }
        }
    });

    // Block on Win32 Message Loop
    WindowManager::runMessageLoop();
    
    WindowManager::cleanupIcon();

    return 0;
}






