#include "pch.h"
#include "LibraryPage.h"
#if __has_include("LibraryPage.g.cpp")
#include "LibraryPage.g.cpp"
#endif

#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <shobjidl_core.h> // IInitializeWithWindow
#include <shlobj.h> // SHGetKnownFolderPath
#include <wincodec.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "apk_extractor/ApkExtractor.h"
#include "androidfw/ResourceTypes.h"
#include "androidfw/AssetManager2.h"
#include "androidfw/ApkAssets.h"
#include "androidfw/ResourceUtils.h"
#include "utils/Logger.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation::Collections;

namespace winrt::SetuShell::implementation
{
    void LibraryPage::LoadApps()
    {
        m_apps.Clear();
        
        PWSTR localAppData = nullptr;
        std::wstring appsDirPath;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
            appsDirPath = std::wstring(localAppData) + L"\\Setu\\Apps";
            CoTaskMemFree(localAppData);
        } else {
            appsDirPath = L"C:\\Setu\\Apps";
        }

        if (!std::filesystem::exists(appsDirPath)) return;

        for (const auto& entry : std::filesystem::directory_iterator(appsDirPath)) {
            if (entry.is_directory()) {
                std::wstring manifestPath = entry.path().wstring() + L"\\manifest.json";
                if (std::filesystem::exists(manifestPath)) {
                    try {
                        std::ifstream file(manifestPath);
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        
                        winrt::Windows::Data::Json::JsonObject json = winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(buffer.str()));
                        
                        hstring pkgName = json.GetNamedString(L"package_name");
                        hstring displayName = json.GetNamedString(L"display_name");
                        hstring iconPath = json.GetNamedString(L"icon_path");
                        
                        std::wstring absoluteIconPath = entry.path().wstring() + L"\\" + iconPath.c_str();
                        
                        m_apps.Append(winrt::make<AppTileViewModel>(pkgName, displayName, winrt::hstring(absoluteIconPath.c_str())));
                    } catch (...) {
                        // ignore broken manifests
                    }
                }
            }
        }
    }

    LibraryPage::LibraryPage()
    {
        m_apps = winrt::single_threaded_observable_vector<SetuShell::AppTileViewModel>();
        
        InitializeComponent();
        LoadApps();
    }

    winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::AppTileViewModel> LibraryPage::Apps()
    {
        return m_apps;
    }

    winrt::Windows::Foundation::IAsyncAction LibraryPage::InstallApk_Click(
        winrt::Windows::Foundation::IInspectable const& sender, 
        winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        auto picker = winrt::Windows::Storage::Pickers::FileOpenPicker();
        picker.ViewMode(winrt::Windows::Storage::Pickers::PickerViewMode::Thumbnail);
        picker.SuggestedStartLocation(winrt::Windows::Storage::Pickers::PickerLocationId::ComputerFolder);
        picker.FileTypeFilter().Append(L".apk");

        // Initialize with window
        HWND hwnd = GetActiveWindow();
        auto initializeWithWindow = picker.as<::IInitializeWithWindow>();
        initializeWithWindow->Initialize(hwnd);

        winrt::Windows::Storage::StorageFile file = co_await picker.PickSingleFileAsync();
        if (file == nullptr) {
            co_return; // user cancelled
        }

        std::string apkPath = winrt::to_string(file.Path());

        // Prepare local app data folder
        std::wstring appsDirPath;
        PWSTR localAppData = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
            appsDirPath = std::wstring(localAppData) + L"\\Setu\\Apps";
            CoTaskMemFree(localAppData);
        } else {
            // Fallback if SHGetKnownFolderPath fails
            appsDirPath = L"C:\\Setu\\Apps";
        }
        std::filesystem::create_directories(appsDirPath);

        std::wstring pkgDir;

        try {
            // 1. Initialize ApkExtractor
            ApkExtractor extractor;
            if (!extractor.OpenApk(apkPath)) {
                Logger::e("InstallPipeline", "Failed to open APK via ApkExtractor");
                co_return;
            }

            // 2. Parse AndroidManifest.xml
            std::vector<uint8_t> axmlBuffer;
            if (!extractor.ExtractEntryToMemory("AndroidManifest.xml", axmlBuffer)) {
                Logger::e("InstallPipeline", "Missing AndroidManifest.xml");
                co_return;
            }

            android::ResXMLTree tree;
            if (tree.setTo(axmlBuffer.data(), axmlBuffer.size(), true) != android::NO_ERROR) {
                Logger::e("InstallPipeline", "Failed to parse AndroidManifest.xml");
                co_return;
            }

            android::ResXMLParser parser(tree);
            parser.restart();

            std::string packageName, versionName, appLabelRef, iconRef;
            int versionCode = 0;

            android::ResXMLParser::event_code_t code;
            while ((code = parser.next()) != android::ResXMLParser::END_DOCUMENT && code != android::ResXMLParser::BAD_DOCUMENT) {
                if (code != android::ResXMLParser::START_TAG) continue;
                
                size_t len;
                const char16_t* tag16 = parser.getElementName(&len);
                if (!tag16) continue;
                std::u16string tag(tag16, len);

                if (tag == u"manifest") {
                    for (size_t i = 0; i < parser.getAttributeCount(); i++) {
                        size_t attrLen;
                        const char16_t* attrName16 = parser.getAttributeName(i, &attrLen);
                        if (!attrName16) continue;
                        std::u16string attrName(attrName16, attrLen);
                        
                        if (attrName == u"package") {
                            size_t valLen;
                            const char16_t* val16 = parser.getAttributeStringValue(i, &valLen);
                            if (val16) packageName = winrt::to_string(std::wstring_view((const wchar_t*)val16, valLen));
                        } else if (attrName == u"versionName") {
                            size_t valLen;
                            const char16_t* val16 = parser.getAttributeStringValue(i, &valLen);
                            if (val16) versionName = winrt::to_string(std::wstring_view((const wchar_t*)val16, valLen));
                        } else if (attrName == u"versionCode") {
                            versionCode = parser.getAttributeData(i);
                        }
                    }
                } else if (tag == u"application") {
                    for (size_t i = 0; i < parser.getAttributeCount(); i++) {
                        size_t attrLen;
                        const char16_t* attrName16 = parser.getAttributeName(i, &attrLen);
                        if (!attrName16) continue;
                        std::u16string attrName(attrName16, attrLen);

                        if (attrName == u"label") {
                            uint32_t type = parser.getAttributeDataType(i);
                            if (type == android::Res_value::TYPE_REFERENCE) {
                                appLabelRef = std::to_string(parser.getAttributeData(i));
                            } else if (type == android::Res_value::TYPE_STRING) {
                                size_t valLen;
                                const char16_t* val16 = parser.getAttributeStringValue(i, &valLen);
                                if (val16) appLabelRef = winrt::to_string(std::wstring_view((const wchar_t*)val16, valLen));
                            }
                        } else if (attrName == u"icon") {
                            uint32_t type = parser.getAttributeDataType(i);
                            if (type == android::Res_value::TYPE_REFERENCE) {
                                iconRef = std::to_string(parser.getAttributeData(i));
                            }
                        }
                    }
                }
            }

            if (packageName.empty()) {
                Logger::e("InstallPipeline", "Package name not found in manifest");
                co_return;
            }

            pkgDir = appsDirPath + L"\\" + winrt::to_hstring(packageName).c_str();
            
            // Handle duplicate packages (overwrite)
            if (std::filesystem::exists(pkgDir)) {
                Logger::i("InstallPipeline", "Package already exists. Overwriting: " + packageName);
                std::filesystem::remove_all(pkgDir);
            }
            std::filesystem::create_directories(pkgDir);

            // 3. Resolve Resources (Label and Icon)
            std::string finalLabel = appLabelRef; // fallback
            std::string finalIconPath = "";

            auto apkAssets = android::ApkAssets::Load(apkPath);
            if (apkAssets) {
                android::AssetManager2 assetManager;
                std::vector<android::AssetManager2::ApkAssetsPtr> assetsList = { apkAssets };
                assetManager.SetApkAssets(assetsList);
                
                android::ResTable_config config;
                memset(&config, 0, sizeof(config));
                config.density = 480; // xxhdpi
                assetManager.SetConfigurations({{config}});

                auto resolveRef = [&](std::string& ref, std::string& out) {
                    if (!ref.empty() && isdigit(ref[0])) {
                        uint32_t resId = std::stoul(ref);
                        auto res = assetManager.GetResource(resId);
                        if (res.has_value()) {
                            android::AssetManager2::SelectedValue val = res.value();
                            if (val.type == android::Res_value::TYPE_REFERENCE) {
                                assetManager.ResolveReference(val);
                            }
                            if (val.type == android::Res_value::TYPE_STRING) {
                                auto pool = assetManager.GetStringPoolForCookie(val.cookie);
                                if (pool) {
                                    auto strExp = pool->stringAt(val.data);
                                    if (strExp.has_value()) out = android::util::Utf16ToUtf8(strExp.value());
                                    else {
                                        auto str8Exp = pool->string8At(val.data);
                                        if (str8Exp.has_value()) out = std::string(str8Exp.value());
                                    }
                                }
                            }
                        }
                    }
                };

                resolveRef(appLabelRef, finalLabel);
                resolveRef(iconRef, finalIconPath);
            }

            // 4. Decode and save Icon using WIC
            std::wstring iconDest = pkgDir + L"\\icon.png";
            bool iconSaved = false;

            if (!finalIconPath.empty()) {
                std::vector<uint8_t> iconBuffer;
                if (extractor.ExtractEntryToMemory(finalIconPath, iconBuffer)) {
                    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                    ::Microsoft::WRL::ComPtr<IWICImagingFactory> wicFactory;
                    if (SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)))) {
                        ::Microsoft::WRL::ComPtr<IWICStream> stream;
                        if (SUCCEEDED(wicFactory->CreateStream(&stream))) {
                            if (SUCCEEDED(stream->InitializeFromMemory(iconBuffer.data(), (DWORD)iconBuffer.size()))) {
                                ::Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
                                if (SUCCEEDED(wicFactory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder))) {
                                    ::Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
                                    if (SUCCEEDED(decoder->GetFrame(0, &frame))) {
                                        ::Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
                                        if (SUCCEEDED(wicFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder))) {
                                            ::Microsoft::WRL::ComPtr<IWICStream> outStream;
                                            if (SUCCEEDED(wicFactory->CreateStream(&outStream))) {
                                                if (SUCCEEDED(outStream->InitializeFromFilename(iconDest.c_str(), GENERIC_WRITE))) {
                                                    if (SUCCEEDED(encoder->Initialize(outStream.Get(), WICBitmapEncoderNoCache))) {
                                                        ::Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> outFrame;
                                                        if (SUCCEEDED(encoder->CreateNewFrame(&outFrame, nullptr))) {
                                                            if (SUCCEEDED(outFrame->Initialize(nullptr))) {
                                                                if (SUCCEEDED(outFrame->WriteSource(frame.Get(), nullptr))) {
                                                                    if (SUCCEEDED(outFrame->Commit()) && SUCCEEDED(encoder->Commit())) {
                                                                        iconSaved = true;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!iconSaved) {
                Logger::i("InstallPipeline", "Icon decode failed or absent, using placeholder.");
                // Create a 1x1 transparent PNG as a placeholder
                std::vector<uint8_t> placeholder = {
                    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // Signature
                    0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, // IHDR header
                    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, // 1x1
                    0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, // 8-bit RGBA
                    0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, // IDAT header
                    0x08, 0xD7, 0x63, 0x60, 0x00, 0x02, 0x00, 0x00, // IDAT chunk
                    0x05, 0x00, 0x01, 0xE2, 0x26, 0x05, 0x9B, 0x00, // IDAT chunk
                    0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, // IEND header
                    0x42, 0x60, 0x82                                // IEND chunk
                };
                std::ofstream ofs(iconDest, std::ios::binary);
                ofs.write(reinterpret_cast<const char*>(placeholder.data()), placeholder.size());
            }

            // 5. Extract APK contents to apk_contents\ and calculate size
            std::wstring apkContentsDir = pkgDir + L"\\apk_contents";
            std::filesystem::create_directories(apkContentsDir);
            
            size_t totalUncompressedSize = 0;
            
            std::vector<ApkEntry> entries = extractor.listEntries();
            for (const auto& entry : entries) {
                totalUncompressedSize += entry.uncompressedSize;
                
                std::string entryName = entry.filename;
                std::wstring entryDest = apkContentsDir + L"\\" + winrt::to_hstring(entryName).c_str();
                
                // Create parent directories
                std::filesystem::path destPath(entryDest);
                if (entryName.empty() || entryName.back() == '/') {
                    std::filesystem::create_directories(destPath);
                    continue;
                }
                std::filesystem::create_directories(destPath.parent_path());
                
                // Extract
                std::string outPathStr = winrt::to_string(entryDest);
                extractor.ExtractEntry(entryName, outPathStr);
            }

            // 6. Write manifest.json
            winrt::Windows::Data::Json::JsonObject json;
            json.SetNamedValue(L"package_name", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(packageName)));
            json.SetNamedValue(L"display_name", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(finalLabel)));
            json.SetNamedValue(L"version_name", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(versionName)));
            json.SetNamedValue(L"version_code", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(versionCode));
            json.SetNamedValue(L"install_path", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"apk_contents\\"));
            json.SetNamedValue(L"icon_path", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"icon.png"));
            
            auto now = winrt::clock::now();
            auto time = winrt::clock::to_time_t(now);
            char buf[64];
            ctime_s(buf, sizeof(buf), &time);
            std::string timeStr(buf);
            if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

            json.SetNamedValue(L"installed_at", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(timeStr)));
            json.SetNamedValue(L"last_run_at", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L""));
            json.SetNamedValue(L"permissions_granted", winrt::Windows::Data::Json::JsonArray());
            json.SetNamedValue(L"install_size_bytes", winrt::Windows::Data::Json::JsonValue::CreateNumberValue((double)totalUncompressedSize));

            std::wstring manifestDest = pkgDir + L"\\manifest.json";
            std::ofstream manifestFile(manifestDest);
            if (manifestFile.is_open()) {
                std::string jsonStr = winrt::to_string(json.Stringify());
                manifestFile.write(jsonStr.c_str(), jsonStr.size());
                manifestFile.close();
            } else {
                throw std::runtime_error("Could not write manifest.json");
            }

            Logger::i("InstallPipeline", "Successfully installed " + packageName);
            LoadApps();

        } catch (const std::exception& ex) {
            Logger::e("InstallPipeline", "Install aborted due to error: " + std::string(ex.what()));
            if (!pkgDir.empty() && std::filesystem::exists(pkgDir)) {
                std::filesystem::remove_all(pkgDir);
            }
        }

        co_return;
    }
}
