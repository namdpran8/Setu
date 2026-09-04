#include "pch.h"
#include "RunningPage.h"
#if __has_include("RunningPage.g.cpp")
#include "RunningPage.g.cpp"
#endif

#include "ProcessManager.h"
#include <chrono>
#include <fstream>
#include <filesystem>
#include <winrt/Windows.Data.Json.h>
#include <shlobj.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::SetuShell::implementation
{
    RunningPage::RunningPage()
    {
        m_runningApps = winrt::single_threaded_observable_vector<SetuShell::RunningAppViewModel>();
        m_dispatcherQueue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
        
        InitializeComponent();
        
        RefreshList();
        
        m_processExitedToken = ProcessManager::Get().AddProcessExitedHandler([this](const std::wstring& package) {
            OnProcessExited(package);
        });
    }

    RunningPage::~RunningPage()
    {
        ProcessManager::Get().RemoveProcessExitedHandler(m_processExitedToken);
    }

    winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::RunningAppViewModel> RunningPage::RunningApps()
    {
        return m_runningApps;
    }

    void RunningPage::RefreshList()
    {
        m_runningApps.Clear();
        
        auto apps = ProcessManager::Get().GetRunningApps();
        
        if (apps.empty()) {
            EmptyStateText().Visibility(Visibility::Visible);
            RunningAppsList().Visibility(Visibility::Collapsed);
        } else {
            EmptyStateText().Visibility(Visibility::Collapsed);
            RunningAppsList().Visibility(Visibility::Visible);
            
            PWSTR localAppData = nullptr;
            std::wstring appsDirPath;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &localAppData))) {
                appsDirPath = std::wstring(localAppData) + L"\\Setu\\Apps\\";
                CoTaskMemFree(localAppData);
            }
            
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            for (const auto& pair : apps) {
                std::wstring package = pair.first;
                
                // Get display name from manifest
                hstring displayName = winrt::hstring(package.c_str());
                std::wstring manifestPath = appsDirPath + package + L"\\manifest.json";
                if (std::filesystem::exists(manifestPath)) {
                    try {
                        std::ifstream file(manifestPath);
                        std::string jsonStr((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                        winrt::Windows::Data::Json::JsonObject json = winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(jsonStr));
                        displayName = json.GetNamedString(L"display_name");
                    } catch (...) {}
                }
                
                int64_t uptimeMs = now - pair.second.launchTimestamp;
                int64_t uptimeSecs = uptimeMs / 1000;
                int64_t uptimeMins = uptimeSecs / 60;
                int64_t uptimeHours = uptimeMins / 60;
                
                std::wstring uptimeStr;
                if (uptimeHours > 0) uptimeStr = std::to_wstring(uptimeHours) + L"h " + std::to_wstring(uptimeMins % 60) + L"m";
                else if (uptimeMins > 0) uptimeStr = std::to_wstring(uptimeMins) + L"m " + std::to_wstring(uptimeSecs % 60) + L"s";
                else uptimeStr = std::to_wstring(uptimeSecs) + L"s";
                
                m_runningApps.Append(winrt::make<RunningAppViewModel>(
                    winrt::hstring(package.c_str()),
                    displayName,
                    winrt::hstring(std::to_wstring(pair.second.pid).c_str()),
                    winrt::hstring(uptimeStr.c_str())
                ));
            }
            
            RunningAppsList().ItemsSource(m_runningApps);
        }
    }

    void RunningPage::OnProcessExited(const std::wstring& package)
    {
        if (m_dispatcherQueue) {
            m_dispatcherQueue.TryEnqueue([this]() {
                RefreshList();
            });
        }
    }

    void RunningPage::ForceStop_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        auto button = sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>();
        auto tag = unbox_value<hstring>(button.Tag());
        
        ProcessManager::Get().ForceStop(tag.c_str());
        // UI will update when the process actually exits and fires the event
    }
}
