#pragma once
#include "RunningPage.g.h"
#include "RunningPage.xaml.g.h"
#include "RunningAppViewModel.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Microsoft.UI.Dispatching.h>

namespace winrt::SetuShell::implementation
{
    struct RunningPage : RunningPageT<RunningPage>
    {
        RunningPage();
        ~RunningPage();

        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::RunningAppViewModel> RunningApps();
        
        void ForceStop_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        void RefreshList();
        void OnProcessExited(const std::wstring& package);
        
        winrt::Windows::Foundation::Collections::IObservableVector<SetuShell::RunningAppViewModel> m_runningApps;
        int m_processExitedToken = 0;
        winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcherQueue{ nullptr };
    };
}
namespace winrt::SetuShell::factory_implementation
{
    struct RunningPage : RunningPageT<RunningPage, implementation::RunningPage>
    {
    };
}
