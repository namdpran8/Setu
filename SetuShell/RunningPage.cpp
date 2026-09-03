#include "pch.h"
#include "RunningPage.h"
#if __has_include("RunningPage.g.cpp")
#include "RunningPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::SetuShell::implementation
{
    RunningPage::RunningPage()
    {
        InitializeComponent();
    }
}
