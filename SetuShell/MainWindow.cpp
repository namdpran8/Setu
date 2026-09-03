#include "pch.h"
#pragma comment(lib, "User32.lib")
#include "MainWindow.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "LibraryPage.h"
#include "RunningPage.h"
#include "SettingsPage.h"

#include <microsoft.ui.xaml.window.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::SetuShell::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
        this->Content(RootGrid());
        

        this->Title(L"Setu");
        // this->ExtendsContentIntoTitleBar(true);

        auto windowNative = this->try_as<IWindowNative>();
        HWND hwnd = nullptr;
        if (windowNative) {
            windowNative->get_WindowHandle(&hwnd);
            SetWindowPos(hwnd, nullptr, 0, 0, 1200, 800, SWP_NOMOVE | SWP_NOZORDER);
        }

        winrt::Microsoft::UI::Xaml::Media::MicaBackdrop backdrop;
        this->SystemBackdrop(backdrop);
    }

    void MainWindow::NavView_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        NavView().SelectedItem(LibraryItem());
    }

    void MainWindow::NavView_SelectionChanged(NavigationView const&, NavigationViewSelectionChangedEventArgs const& args)
    {
        if (args.IsSettingsSelected())
        {
            ContentFrame().Navigate(xaml_typename<SetuShell::SettingsPage>());
        }
        else
        {
            auto item = args.SelectedItem().as<NavigationViewItem>();
            auto tag = unbox_value<hstring>(item.Tag());
            if (tag == L"LibraryPage")
            {
                ContentFrame().Navigate(xaml_typename<SetuShell::LibraryPage>());
            }
            else if (tag == L"RunningPage")
            {
                ContentFrame().Navigate(xaml_typename<SetuShell::RunningPage>());
            }
        }
    }
}
