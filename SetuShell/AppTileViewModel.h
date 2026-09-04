#pragma once
#include "AppTileViewModel.g.h"
namespace winrt::SetuShell::implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel>
    {
        AppTileViewModel(hstring const& packageName, hstring const& name, hstring const& iconPath) : m_packageName(packageName), m_name(name), m_iconPath(iconPath) {}
        hstring PackageName() { return m_packageName; }
        hstring Name() { return m_name; }
        hstring IconPath() { return m_iconPath; }
    private:
        hstring m_packageName;
        hstring m_name;
        hstring m_iconPath;
    };
}

namespace winrt::SetuShell::factory_implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel, implementation::AppTileViewModel>
    {
    };
}
