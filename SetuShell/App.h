#pragma once
#include "App.g.h"
#include "App.xaml.g.h"
namespace winrt::SetuShell::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}

namespace winrt::SetuShell::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
