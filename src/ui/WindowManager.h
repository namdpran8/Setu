#pragma once
#include <windows.h>
#include <string>
#include <functional>
#include <memory>
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <dxgi1_2.h>
#include <d3d11_1.h>
#include <wrl/client.h>

namespace setu {
namespace view {
    class View;
}
}

class WindowManager {
public:
    static bool init();
    static void runMessageLoop();
    static HWND getMainWindow();

    // D2D getters
    static ID2D1DeviceContext* getD2DContext();
    static IDWriteFactory* getDWriteFactory();
    static IDXGISwapChain1* getSwapChain();

    // Render loop integration
    static void beginDraw();
    static void endDraw();

    static void clearWindow();
    static void setClickCallback(std::function<void(int)> cb);
    static void triggerClickCallback(int controlId);

    // C++ View Hierarchy root
    static void setRootView(std::shared_ptr<setu::view::View> rootView);
    static std::shared_ptr<setu::view::View> getRootView();
    static void dumpRootViewAfterLayout();

    // Forwarders. The values themselves live on setu::view::View, because
    // ViewGroup has to scale a layout_margin and the view layer is also built
    // standalone (constraint_layout_test), where no WindowManager is linked.
    // Keeping these as the public spelling means every existing caller -
    // XmlAttrs, TextView - is untouched. Defined out of line because View is
    // only forward-declared here.
    static float getDensity();
    static void setDensity(float density);

    static float getScaledDensity();
    static void setScaledDensity(float scaledDensity);

private:
    static std::shared_ptr<setu::view::View> s_rootView;
    static bool s_rootViewDumpPending;
    static std::function<void(int)> s_clickCallback;
    static HWND s_mainWindow;

    // Direct2D / DirectX resources
    static Microsoft::WRL::ComPtr<ID3D11Device> s_d3dDevice;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext> s_d3dContext;
    static Microsoft::WRL::ComPtr<IDXGISwapChain1> s_swapChain;
    static Microsoft::WRL::ComPtr<ID2D1Factory1> s_d2dFactory;
    static Microsoft::WRL::ComPtr<ID2D1Device> s_d2dDevice;
    static Microsoft::WRL::ComPtr<ID2D1DeviceContext> s_d2dContext;
    static Microsoft::WRL::ComPtr<IDWriteFactory> s_dWriteFactory;

    static bool initDirect2D();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
