#pragma once
#include "LibraryPage.g.h"
#include "LibraryPage.xaml.g.h"
#include "AppTileViewModel.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::SetuShell::implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage>
    {
        LibraryPage();
        ~LibraryPage();
        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::AppTileViewModel> Apps();
        winrt::Windows::Foundation::IAsyncAction InstallApk_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppTile_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e);
        
        void SearchBox_TextChanged(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& args);
        void AppTileMenu_Opening(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);
        void AppTile_Launch(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppTile_ForceStop(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppTile_ShowExplorer(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppTile_ViewPermissions(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppTile_Uninstall(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        std::vector<SetuShell::AppTileViewModel> m_allApps;
        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::AppTileViewModel> m_apps;
        void LoadApps();
        
        int m_launchedHandlerToken = 0;
        int m_exitedHandlerToken = 0;
        
        void UpdateRunningState(const std::wstring& package, bool isRunning);
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage, implementation::LibraryPage>
    {
    };
}
