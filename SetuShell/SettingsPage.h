#pragma once
#include "SettingsPage.g.h"
#include "SettingsPage.xaml.g.h"

namespace winrt::SetuShell::implementation
{
    struct SettingsPage : SettingsPageT<SettingsPage>
    {
        SettingsPage();
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct SettingsPage : SettingsPageT<SettingsPage, implementation::SettingsPage>
    {
    };
}
