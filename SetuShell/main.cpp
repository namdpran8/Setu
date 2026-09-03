#include "pch.h"
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <microsoft.ui.xaml.window.h>
#include <MddBootstrap.h>
#include <appmodel.h>
#pragma comment(lib, "Microsoft.WindowsAppRuntime.Bootstrap.lib")

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;

struct App : ApplicationT<App>
{
    void OnLaunched(LaunchActivatedEventArgs const&)
    {
        window = Window();
        window.Title(L"Setu");
        
        auto windowNative = window.as<IWindowNative>();
        HWND hwnd = nullptr;
        windowNative->get_WindowHandle(&hwnd);
        SetWindowPos(hwnd, nullptr, 0, 0, 1200, 800, SWP_NOMOVE | SWP_NOZORDER);
        
        MicaBackdrop backdrop;
        window.SystemBackdrop(backdrop);

        Grid grid;
        window.Content(grid);

        window.Activate();
    }
    Window window{ nullptr };
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    init_apartment(apartment_type::single_threaded);

    PACKAGE_VERSION minVersion{};
    minVersion.Version = 0;
#if !defined(MICROSOFT_WINDOWSAPPSDK_SELFCONTAINED)
    HRESULT hr = MddBootstrapInitialize2(0x00010005, L"", minVersion, MddBootstrapInitializeOptions_OnNoMatch_ShowUI);
    if (FAILED(hr)) {
        return 1;
    }
#endif

    Application::Start([](auto&&) { make<App>(); });

#if !defined(MICROSOFT_WINDOWSAPPSDK_SELFCONTAINED)
    MddBootstrapShutdown();
#endif
    return 0;
}
