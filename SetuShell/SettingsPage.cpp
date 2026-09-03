#include "pch.h"
#include "SettingsPage.h"
#if __has_include("SettingsPage.g.cpp")
#include "SettingsPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::SetuShell::implementation
{
    SettingsPage::SettingsPage()
    {
        InitializeComponent();
    }
}
