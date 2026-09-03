#pragma once
#include "RunningPage.g.h"
#include "RunningPage.xaml.g.h"

namespace winrt::SetuShell::implementation
{
    struct RunningPage : RunningPageT<RunningPage>
    {
        RunningPage();
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct RunningPage : RunningPageT<RunningPage, implementation::RunningPage>
    {
    };
}
