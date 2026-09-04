#pragma once
#include "AppTileViewModel.g.h"
namespace winrt::SetuShell::implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel>
    {
        AppTileViewModel(hstring const& packageName, hstring const& name, hstring const& iconPath, hstring const& installPath) : m_packageName(packageName), m_name(name), m_iconPath(iconPath), m_installPath(installPath) {}
        hstring PackageName() { return m_packageName; }
        hstring Name() { return m_name; }
        hstring IconPath() { return m_iconPath; }
        hstring InstallPath() { return m_installPath; }
    private:
        hstring m_packageName;
        hstring m_name;
        hstring m_iconPath;
        hstring m_installPath;
    };
}

namespace winrt::SetuShell::factory_implementation
{
    struct AppTileViewModel : AppTileViewModelT<AppTileViewModel, implementation::AppTileViewModel>
    {
    };
}
