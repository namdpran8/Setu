#pragma once
#include "RunningAppViewModel.g.h"
namespace winrt::SetuShell::implementation
{
    struct RunningAppViewModel : RunningAppViewModelT<RunningAppViewModel>
    {
        RunningAppViewModel(hstring const& packageName, hstring const& name, hstring const& pidStr, hstring const& uptimeStr) : m_packageName(packageName), m_name(name), m_pidStr(pidStr), m_uptimeStr(uptimeStr) {}
        hstring PackageName() { return m_packageName; }
        hstring Name() { return m_name; }
        hstring PidStr() { return m_pidStr; }
        hstring UptimeStr() { return m_uptimeStr; }
    private:
        hstring m_packageName;
        hstring m_name;
        hstring m_pidStr;
        hstring m_uptimeStr;
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct RunningAppViewModel : RunningAppViewModelT<RunningAppViewModel, implementation::RunningAppViewModel>
    {
    };
}