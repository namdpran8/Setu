#include "androidfw/AssetManager2.h"
#include "androidfw/ApkAssets.h"
#include "androidfw/ResourceUtils.h"
#include "androidfw/StringPool.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace android;

int main() {
    std::cout << "test_androidfw starting..." << std::endl;
    
    ApkAssetsPtr apk = ApkAssets::Load("C:\\Users\\namde\\Documents\\Windroid\\app-release.apk");
    if (!apk) {
        std::cerr << "Failed to load APK!" << std::endl;
        return 1;
    }
    
    AssetManager2 am;
    am.SetApkAssets({apk});
    
    auto res_id_exp = am.GetResourceId("app_name", "string", "com.darkempire78.opencalculator");
    if (!res_id_exp.has_value()) {
        std::cerr << "Could not find resource ID for app_name!" << std::endl;
    } else {
        uint32_t res_id = res_id_exp.value();
        std::cout << "Found app_name res_id: 0x" << std::hex << res_id << std::endl;
        auto value = am.GetResource(res_id);
        if (value.has_value()) {
            std::cout << "Resolved value! type: " << (int)value->type << ", data: " << value->data << std::endl;
            if (value->type == Res_value::TYPE_STRING) {
                auto str_exp = am.GetStringPoolForCookie(value->cookie)->stringAt(value->data);
                if (str_exp.has_value()) {
                    auto str = str_exp.value();
                    std::cout << "String length: " << str.size() << std::endl;
                    std::wcout << L"String value: " << std::wstring((const wchar_t*)str.data(), str.size()) << std::endl;
                }
            }
        } else {
            std::cerr << "Could not resolve resource value!" << std::endl;
        }
    }
    
    return 0;
}
