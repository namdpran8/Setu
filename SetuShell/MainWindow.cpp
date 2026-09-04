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
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>

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
        this->ExtendsContentIntoTitleBar(true);

        auto windowNative = this->try_as<IWindowNative>();
        HWND hwnd = nullptr;
        if (windowNative) {
            windowNative->get_WindowHandle(&hwnd);
            SetWindowPos(hwnd, nullptr, 0, 0, 1000, 600, SWP_NOMOVE | SWP_NOZORDER);

            winrt::Microsoft::UI::WindowId windowId = winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd);
            winrt::Microsoft::UI::Windowing::AppWindow appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(windowId);
            
            wchar_t modulePath[MAX_PATH];
            GetModuleFileNameW(NULL, modulePath, MAX_PATH);
            std::wstring moduleDir = modulePath;
            size_t lastSlash = moduleDir.find_last_of(L"\\/");
            if (lastSlash != std::wstring::npos) {
                moduleDir = moduleDir.substr(0, lastSlash);
            }
            std::wstring iconPath = moduleDir + L"\\..\\..\\..\\..\\assests\\icon\\setu_icon.ico";
            appWindow.SetIcon(iconPath.c_str());
            
            HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(101));
            if (hIcon) {
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
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
