#include "androidfw/AssetManager2.h"
#include "androidfw/ApkAssets.h"
#include "androidfw/ResourceUtils.h"
#include "androidfw/StringPool.h"
#include "androidfw/LoadedArsc.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace android;

int main(int argc, char** argv) {
    std::cout << "test_androidfw starting..." << std::endl;
    
    std::string apkPath = "testapk\\openclalc.apk";
    if (argc > 1) {
        apkPath = argv[1];
    }
    ApkAssetsPtr apk = ApkAssets::Load(apkPath);
    if (!apk) {
        std::cerr << "Failed to load APK!" << std::endl;
        return 1;
    }
    
    const LoadedArsc* loaded_arsc = apk->GetLoadedArsc();
    if (loaded_arsc) {
        std::cout << "Loaded ARSC from APK. Package count: " << loaded_arsc->GetPackages().size() << std::endl;
        for (const auto& pkg : loaded_arsc->GetPackages()) {
            std::cout << "Package: " << pkg->GetPackageName() << " (ID: 0x" << std::hex << pkg->GetPackageId() << ")" << std::endl;
        }
    } else {
        std::cerr << "No LoadedArsc found in ApkAssets!" << std::endl;
    }
    
    AssetManager2 am;
    am.SetApkAssets({apk});
    
    // Try to resolve the first string in the package.
    // Usually package id is 0x7f, string type might be 0x01 or 0x02 etc.
    // Let's just iterate over some possible resource IDs to see what resolves.
    for (uint32_t res_id = 0x7f010000; res_id < 0x7f100000; ++res_id) {
        auto value = am.GetResource(res_id);
        if (value.has_value()) {
            auto name_exp = am.GetResourceName(res_id);
            if (name_exp.has_value()) {
                const auto& name = name_exp.value();
                std::string formatted_name = android::ToFormattedResourceString(name);
                std::cout << "Found resource 0x" << std::hex << res_id 
                          << " : " << formatted_name << std::endl;
                
                if (value->type == Res_value::TYPE_STRING) {
                    auto str_exp = am.GetStringPoolForCookie(value->cookie)->stringAt(value->data);
                    if (str_exp.has_value()) {
                        auto str = str_exp.value();
                        std::wcout << L"  -> String value: " << std::wstring((const wchar_t*)str.data(), str.size()) << std::endl;
                    }
                }
                break; // Stop after finding the first valid resource
            }
        }
    }
    
    return 0;
}
