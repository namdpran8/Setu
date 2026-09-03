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
        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::AppTileViewModel> Apps();
    private:
        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::AppTileViewModel> m_apps;
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct LibraryPage : LibraryPageT<LibraryPage, implementation::LibraryPage>
    {
    };
}
