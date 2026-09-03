#include "pch.h"
#include "LibraryPage.h"
#if __has_include("LibraryPage.g.cpp")
#include "LibraryPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Windows::Foundation::Collections;

namespace winrt::SetuShell::implementation
{
    LibraryPage::LibraryPage()
    {
        m_apps = winrt::single_threaded_observable_vector<SetuShell::AppTileViewModel>();
        m_apps.Append(winrt::make<AppTileViewModel>(L"Camera", L"\xE722"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Messages", L"\xE8BD"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Maps", L"\xE707"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Notes", L"\xE7C6"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Gallery", L"\xE8B9"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Music", L"\xE8D6"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Files", L"\xE8B7"));
        m_apps.Append(winrt::make<AppTileViewModel>(L"Calendar", L"\xE787"));

        InitializeComponent();
        

    }

    IObservableVector<SetuShell::AppTileViewModel> LibraryPage::Apps()
    {
        return m_apps;
    }
}
