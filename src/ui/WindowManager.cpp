#include "WindowManager.h"
#include "../utils/Logger.h"
#include "../view/View.h"
#include "../view/MotionEvent.h"
#include "../view/Choreographer.h"
#include "../graphics/Direct2DCanvas.h"

HWND WindowManager::s_mainWindow = nullptr;
std::function<void(int)> WindowManager::s_clickCallback = nullptr;
std::shared_ptr<windroid::view::View> WindowManager::s_rootView = nullptr;

Microsoft::WRL::ComPtr<ID3D11Device> WindowManager::s_d3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> WindowManager::s_d3dContext;
Microsoft::WRL::ComPtr<IDXGISwapChain1> WindowManager::s_swapChain;
Microsoft::WRL::ComPtr<ID2D1Factory1> WindowManager::s_d2dFactory;
Microsoft::WRL::ComPtr<ID2D1Device> WindowManager::s_d2dDevice;
Microsoft::WRL::ComPtr<ID2D1DeviceContext> WindowManager::s_d2dContext;
Microsoft::WRL::ComPtr<IDWriteFactory> WindowManager::s_dWriteFactory;

void WindowManager::setClickCallback(std::function<void(int)> cb) {
    s_clickCallback = cb;
}

void WindowManager::setRootView(std::shared_ptr<windroid::view::View> rootView) {
    s_rootView = rootView;
    if (s_mainWindow) {
        InvalidateRect(s_mainWindow, nullptr, FALSE);
    }
}

std::shared_ptr<windroid::view::View> WindowManager::getRootView() {
    return s_rootView;
}

void WindowManager::clearWindow() {
    if (s_mainWindow) {
        EnumChildWindows(s_mainWindow, [](HWND hwnd, LPARAM lParam) -> BOOL {
            DestroyWindow(hwnd);
            return TRUE;
        }, 0);
        InvalidateRect(s_mainWindow, nullptr, TRUE);
        UpdateWindow(s_mainWindow);
        Logger::i("WindowManager", "Cleared all child controls from main window.");
    }
}

bool WindowManager::initDirect2D() {
    // 1. Create Direct3D 11 device
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, creationFlags, 
        featureLevels, ARRAYSIZE(featureLevels), 
        D3D11_SDK_VERSION, &s_d3dDevice, nullptr, &s_d3dContext
    );
    if (FAILED(hr)) return false;

    // 2. Get DXGI Factory
    Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
    hr = s_d3dDevice.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIFactory2> dxgiFactory;
    hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(hr)) return false;

    // 3. Create Swap Chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.Width = 0; // Use window width
    swapChainDesc.Height = 0; // Use window height
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2; // Double buffering
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDesc.Flags = 0;

    hr = dxgiFactory->CreateSwapChainForHwnd(
        s_d3dDevice.Get(),
        s_mainWindow,
        &swapChainDesc,
        nullptr,
        nullptr,
        &s_swapChain
    );
    if (FAILED(hr)) return false;

    // 4. Create Direct2D Factory & Device Context
    D2D1_FACTORY_OPTIONS options;
    options.debugLevel = D2D1_DEBUG_LEVEL_NONE;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &options, (void**)&s_d2dFactory);
    if (FAILED(hr)) return false;

    hr = s_d2dFactory->CreateDevice(dxgiDevice.Get(), &s_d2dDevice);
    if (FAILED(hr)) return false;

    hr = s_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &s_d2dContext);
    if (FAILED(hr)) return false;

    // 5. Create Direct2D Bitmap from SwapChain
    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer;
    hr = s_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
    if (FAILED(hr)) return false;

    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        96.0f, 96.0f // DPI
    );

    Microsoft::WRL::ComPtr<ID2D1Bitmap1> d2dTargetBitmap;
    hr = s_d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer.Get(), &bitmapProperties, &d2dTargetBitmap);
    if (FAILED(hr)) return false;

    s_d2dContext->SetTarget(d2dTargetBitmap.Get());

    // 6. Create DirectWrite Factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&s_dWriteFactory);
    if (FAILED(hr)) return false;

    return true;
}

void WindowManager::beginDraw() {
    if (s_d2dContext) {
        s_d2dContext->BeginDraw();
    }
}

void WindowManager::endDraw() {
    if (s_d2dContext) {
        s_d2dContext->EndDraw();
        s_swapChain->Present(1, 0); // VSync
    }
}

ID2D1DeviceContext* WindowManager::getD2DContext() {
    return s_d2dContext.Get();
}

IDWriteFactory* WindowManager::getDWriteFactory() {
    return s_dWriteFactory.Get();
}

IDXGISwapChain1* WindowManager::getSwapChain() {
    return s_swapChain.Get();
}

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                HWND controlHwnd = (HWND)lParam;
                int controlId = (int)GetWindowLongPtr(controlHwnd, GWLP_ID);
                Logger::d("WindowManager", "Button clicked with ID: " + std::to_string(controlId));
                if (s_clickCallback) {
                    s_clickCallback(controlId);
                }
            }
            return 0;
        case WM_LBUTTONDOWN: {
            if (s_rootView) {
                float x = (float)LOWORD(lParam);
                float y = (float)HIWORD(lParam);
                windroid::view::MotionEvent event(windroid::view::MotionEvent::Action::DOWN, x, y);
                s_rootView->dispatchTouchEvent(event);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (s_rootView) {
                float x = (float)LOWORD(lParam);
                float y = (float)HIWORD(lParam);
                windroid::view::MotionEvent event(windroid::view::MotionEvent::Action::UP, x, y);
                s_rootView->dispatchTouchEvent(event);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (s_rootView) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                windroid::graphics::Direct2DCanvas canvas(s_d2dContext.Get(), s_dWriteFactory.Get());
                windroid::view::Choreographer::getInstance().doFrame(s_rootView, canvas, rect.right, rect.bottom);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

LRESULT CALLBACK ViewGroupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_COMMAND) {
        // Forward button clicks and commands from children to the main window
        return SendMessage(GetParent(hwnd), msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool WindowManager::init() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "WindroidMainWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassEx(&wc)) {
        Logger::e("WindowManager", "Failed to register main window class!");
        return false;
    }
    
    WNDCLASSEX wcGroup = {0};
    wcGroup.cbSize = sizeof(WNDCLASSEX);
    wcGroup.lpfnWndProc = ViewGroupProc;
    wcGroup.hInstance = hInstance;
    wcGroup.lpszClassName = "WindroidViewGroup";
    wcGroup.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent
    wcGroup.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassEx(&wcGroup)) {
        Logger::e("WindowManager", "Failed to register view group window class!");
        return false;
    }
    
    s_mainWindow = CreateWindowEx(
        0,
        "WindroidMainWindow",
        "Windroid Runtime (Direct2D RenderNode)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        540, 1170, // Average modern phone aspect ratio
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!s_mainWindow) {
        Logger::e("WindowManager", "Failed to create main window!");
        return false;
    }
    
    if (!initDirect2D()) {
        Logger::e("WindowManager", "Failed to initialize Direct2D and DXGI!");
        return false;
    }
    
    Logger::i("WindowManager", "Initialized main window and Direct2D successfully.");
    return true;
}

void WindowManager::runMessageLoop() {
    Logger::i("WindowManager", "Starting message loop...");
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Logger::i("WindowManager", "Message loop ended.");
}

HWND WindowManager::getMainWindow() {
    return s_mainWindow;
}
