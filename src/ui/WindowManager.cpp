/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "WindowManager.h"
#include "../os/Looper.h"
#include "../utils/Logger.h"
#include "../view/View.h"
#include "../view/MotionEvent.h"
#include "../view/KeyEvent.h"
#include "../view/Choreographer.h"
#include "../graphics/Direct2DCanvas.h"
#include "../graphics/SkiaCanvas.h"
#include "../graphics/FontManager.h"

static const int KONAMI_CODE[] = {VK_UP, VK_UP, VK_DOWN, VK_DOWN, VK_LEFT, VK_RIGHT, VK_LEFT, VK_RIGHT, 'B', 'A'};
static int s_konamiIndex = 0;
static bool s_showBsod = false;
static int s_renderMode = 1; // 0 = D2D, 1 = Skia, 2 = Side-by-Side

// Timer IDs. The ten-minute idle "ghost touch" already owned 1, and SetTimer
// replaces rather than adds when an ID repeats, so the animation clock needs its
// own or arming one would silently cancel the other.
static const UINT_PTR TIMER_IDLE_GHOST = 1;
static const UINT_PTR TIMER_ANIMATION = 2;

// ~60fps. SetTimer's real resolution is coarser than this and WM_TIMER is a
// low-priority message besides, so it is a floor on the frame interval rather
// than a promise - which is exactly why animations read uptimeMillis() instead of
// stepping themselves once per tick.
static const UINT TIMER_ANIMATION_INTERVAL_MS = 16;
static bool s_animationTimerRunning = false;

HWND WindowManager::s_mainWindow = nullptr;
HWND WindowManager::s_skiaWindow = nullptr;
std::function<void(int)> WindowManager::s_clickCallback = nullptr;
std::function<bool(int)> WindowManager::s_longClickCallback = nullptr;
std::shared_ptr<setu::view::View> WindowManager::s_rootView = nullptr;
bool WindowManager::s_rootViewDumpPending = false;

// Density lives on setu::view::View; see the declarations in WindowManager.h.
float WindowManager::getDensity() {
    return setu::view::View::getDisplayDensity();
}

void WindowManager::setDensity(float density) {
    setu::view::View::setDisplayMetrics(density, getScaledDensity());
}

float WindowManager::getScaledDensity() {
    return setu::view::View::getScaledDensity();
}

void WindowManager::setScaledDensity(float scaledDensity) {
    setu::view::View::setDisplayMetrics(getDensity(), scaledDensity);
}

Microsoft::WRL::ComPtr<ID3D11Device> WindowManager::s_d3dDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> WindowManager::s_d3dContext;
Microsoft::WRL::ComPtr<IDXGISwapChain1> WindowManager::s_swapChain;
Microsoft::WRL::ComPtr<ID2D1Bitmap1> WindowManager::s_d2dTargetBitmap;
Microsoft::WRL::ComPtr<IDXGISwapChain1> WindowManager::s_skiaSwapChain;
Microsoft::WRL::ComPtr<ID2D1Bitmap1> WindowManager::s_skiaTargetBitmap;
Microsoft::WRL::ComPtr<ID2D1Factory1> WindowManager::s_d2dFactory;
Microsoft::WRL::ComPtr<ID2D1Device> WindowManager::s_d2dDevice;
Microsoft::WRL::ComPtr<ID2D1DeviceContext> WindowManager::s_d2dContext;
Microsoft::WRL::ComPtr<IDWriteFactory> WindowManager::s_dWriteFactory;

void WindowManager::setClickCallback(std::function<void(int)> cb) {
    s_clickCallback = cb;
}

void WindowManager::setLongClickCallback(std::function<bool(int)> cb) {
    s_longClickCallback = cb;
}

bool WindowManager::triggerLongClickCallback(int controlId) {
    if (s_longClickCallback) {
        return s_longClickCallback(controlId);
    }
    return false;
}

void WindowManager::triggerClickCallback(int controlId) {
    Logger::d("WindowManager", "triggerClickCallback called for id: " + std::to_string(controlId));
    if (s_clickCallback) {
        s_clickCallback(controlId);
    } else {
        Logger::w("WindowManager", "s_clickCallback is null!");
    }
}

void WindowManager::setRootView(std::shared_ptr<setu::view::View> rootView) {
    if (s_rootView) {
        s_rootView->dispatchDetachedFromWindow();
    }
    s_rootView = rootView;
    if (s_rootView) {
        s_rootView->dispatchAttachedToWindow();
    }
    s_rootViewDumpPending = s_rootView != nullptr;
    if (s_mainWindow) {
        InvalidateRect(s_mainWindow, nullptr, FALSE);
    }
}

std::shared_ptr<setu::view::View> WindowManager::getRootView() {
    return s_rootView;
}

void WindowManager::dumpRootViewAfterLayout() {
    if (!s_rootViewDumpPending || !s_rootView) return;
    s_rootViewDumpPending = false;
    Logger::i("WindowManager", "--- VIEW HIERARCHY DUMP AFTER ROOT LAYOUT ---");
    s_rootView->dump(0);
    Logger::i("WindowManager", "--- VIEW HIERARCHY DUMP END ---");
}

void WindowManager::clearWindow() {
    if (s_mainWindow) {
        // Another one bites the dust...
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

    // 3. Create Swap Chains
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

    hr = dxgiFactory->CreateSwapChainForHwnd(
        s_d3dDevice.Get(),
        s_skiaWindow,
        &swapChainDesc,
        nullptr,
        nullptr,
        &s_skiaSwapChain
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

    // 5. Create Direct2D Bitmaps from SwapChains
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
        96.0f, 96.0f // DPI
    );

    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer1;
    hr = s_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer1));
    if (FAILED(hr)) return false;
    hr = s_d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer1.Get(), &bitmapProperties, &s_d2dTargetBitmap);
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGISurface> dxgiBackBuffer2;
    hr = s_skiaSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer2));
    if (FAILED(hr)) return false;
    hr = s_d2dContext->CreateBitmapFromDxgiSurface(dxgiBackBuffer2.Get(), &bitmapProperties, &s_skiaTargetBitmap);
    if (FAILED(hr)) return false;

    // 6. Create DirectWrite Factory
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&s_dWriteFactory);
    if (FAILED(hr)) return false;

    // Hand it to FontManager immediately: views can measure text during layout
    // inflation, long before the first WM_PAINT builds a canvas.
    setu::graphics::FontManager::getInstance().setFactory(s_dWriteFactory.Get());

    // Close the loop between View::invalidate() and the screen. Until now
    // invalidate() only marked render nodes dirty, and widgets that wanted a
    // repaint had to call InvalidateRect() themselves - which meant anything
    // that changed appearance without knowing about the HWND simply never
    // redrew. Reads s_mainWindow lazily so install order does not matter.
    setu::view::View::setInvalidateHandler([]() {
        if (s_mainWindow) {
            InvalidateRect(s_mainWindow, nullptr, FALSE);
        }
        if (s_skiaWindow) {
            InvalidateRect(s_skiaWindow, nullptr, FALSE);
        }
    });

    // And the frame clock, for drawables that animate rather than just change.
    // Called only when the animation queue goes from empty to non-empty, so a UI
    // at rest runs no timer at all; WM_TIMER stops the clock again on the frame
    // the last animation finishes.
    setu::view::View::setAnimationHandler([]() {
        if (s_mainWindow && !s_animationTimerRunning) {
            SetTimer(s_mainWindow, TIMER_ANIMATION, TIMER_ANIMATION_INTERVAL_MS, nullptr);
            s_animationTimerRunning = true;
        }
    });

    return true;
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
                case WM_LOOPER_WAKE:
            pumpLooper();
            return 0;
        case WM_TIMER:
            if (wParam == TIMER_LOOPER) {
                KillTimer(hwnd, TIMER_LOOPER);
                pumpLooper();
                return 0;
            }
            if (wParam == TIMER_IDLE_GHOST) {
                Logger::i("IdleGhost", "Are you still there? (Ghost Touch)");
                if (s_rootView) {
                    setu::view::MotionEvent eventDown(setu::view::MotionEvent::Action::DOWN, 100, 100);
                    s_rootView->dispatchTouchEvent(eventDown);
                    setu::view::MotionEvent eventUp(setu::view::MotionEvent::Action::UP, 100, 100);
                    s_rootView->dispatchTouchEvent(eventUp);
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
            }
            return 0;
        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            if (width == 404 && height == 404) {
                SetWindowTextA(hwnd, "404 Android Not Found");
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        case WM_LBUTTONDOWN: {
            SetTimer(hwnd, TIMER_IDLE_GHOST, 600000, nullptr); // Reset 10 min idle timer
            if (s_rootView) {
                float x = (float)LOWORD(lParam);
                float y = (float)HIWORD(lParam);
                setu::view::MotionEvent event(setu::view::MotionEvent::Action::DOWN, x, y);
                s_rootView->dispatchTouchEvent(event);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (s_rootView) {
                float x = (float)LOWORD(lParam);
                float y = (float)HIWORD(lParam);
                setu::view::MotionEvent event(setu::view::MotionEvent::Action::UP, x, y);
                s_rootView->dispatchTouchEvent(event);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_KEYDOWN: {
            if (wParam == KONAMI_CODE[s_konamiIndex]) {
                s_konamiIndex++;
                if (s_konamiIndex == sizeof(KONAMI_CODE)/sizeof(int)) {
                    MessageBoxA(hwnd, "Cheat Activated: Infinite RAM", "Konami Code", MB_OK);
                    s_konamiIndex = 0;
                }
            } else {
                s_konamiIndex = 0;
            }
            if (wParam == 'B' && (GetKeyState(VK_CONTROL) & 0x8000) && (GetKeyState(VK_MENU) & 0x8000)) {
                s_showBsod = true;
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            if (s_rootView) {
                if (wParam == VK_F8) {
                    Logger::i("WindowManager", "--- VIEW HIERARCHY DUMP START ---");
                    s_rootView->dump(0);
                    Logger::i("WindowManager", "--- VIEW HIERARCHY DUMP END ---");
                } else if (wParam == VK_F9) {
                    MessageBoxA(hwnd, "You found the hidden Setu Easter Egg!\n\nDalvik says hello from the grave... \xE2\x98\xA0\xEF\xB8\x8F", "Secret Discovered!", MB_OK | MB_ICONINFORMATION);
                    Logger::i("EasterEgg", "User pressed F9! Pshhh...");
                } else if (wParam == VK_F11) {
                    s_renderMode = (s_renderMode + 1) % 3;
                    std::string modeStr;
                    if (s_renderMode == 0) modeStr = "Direct2D";
                    else if (s_renderMode == 1) modeStr = "Skia";
                    else modeStr = "Side-by-Side (D2D | Skia)";
                    
                    Logger::i("WindowManager", "Render mode switched to: " + modeStr);
                    InvalidateRect(hwnd, nullptr, TRUE);
                }
                setu::view::KeyEvent event(setu::view::KeyEvent::Action::DOWN, (int)wParam, 0);
                if (s_rootView->dispatchKeyEvent(event)) {
                    // Handled
                }
            }
            return 0;
        }
        case WM_CHAR: {
            if (s_rootView) {
                setu::view::KeyEvent event(setu::view::KeyEvent::Action::DOWN, 0, (wchar_t)wParam);
                if (s_rootView->dispatchKeyEvent(event)) {
                    // Handled
                }
            }
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (s_showBsod) {
                if (s_d2dContext) {
                    s_d2dContext->SetTarget(hwnd == s_mainWindow ? s_d2dTargetBitmap.Get() : s_skiaTargetBitmap.Get());
                    s_d2dContext->BeginDraw();
                    s_d2dContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.7f)); // Blue background
                    
                    // Draw BSOD Text
                    if (s_dWriteFactory) {
                        Microsoft::WRL::ComPtr<IDWriteTextFormat> textFormat;
                        s_dWriteFactory->CreateTextFormat(
                            L"Consolas", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, 
                            DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &textFormat
                        );
                        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
                        s_d2dContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush);
                        
                        std::wstring bsodText = L"A fatal exception 0E has occurred at 0028:C0011E36 in UXD Dalvik(01) +\n"
                                                L"00010E36. The current application will be terminated.\n\n"
                                                L"*  Press any key to terminate the current application.\n"
                                                L"*  Press CTRL+ALT+DEL again to restart your computer. You will\n"
                                                L"   lose any unsaved information in all applications.\n\n"
                                                L"                  Press any key to continue _";
                        D2D1_RECT_F layoutRect = D2D1::RectF(50.0f, 50.0f, (float)rect.right - 50.0f, (float)rect.bottom - 50.0f);
                        s_d2dContext->DrawText(bsodText.c_str(), (UINT32)bsodText.length(), textFormat.Get(), layoutRect, whiteBrush.Get());
                    }
                    
                    s_d2dContext->EndDraw();
                    if (hwnd == s_mainWindow) s_swapChain->Present(1, 0);
                    else s_skiaSwapChain->Present(1, 0);
                }
            } else if (s_rootView) {
                if (hwnd == s_mainWindow) {
                    s_d2dContext->SetTarget(s_d2dTargetBitmap.Get());
                    s_d2dContext->BeginDraw();
                    
                    setu::graphics::Direct2DCanvas canvas(s_d2dContext.Get(), s_dWriteFactory.Get());
                    setu::view::Choreographer::getInstance().doFrame(s_rootView, canvas, rect.right, rect.bottom);
                    
                    s_d2dContext->EndDraw();
                    s_swapChain->Present(1, 0);
                } else if (hwnd == s_skiaWindow) {
                    s_d2dContext->SetTarget(s_skiaTargetBitmap.Get());
                    s_d2dContext->BeginDraw();
                    
                    setu::graphics::SkiaCanvas canvas(rect.right, rect.bottom);
                    setu::view::Choreographer::getInstance().doFrame(s_rootView, canvas, rect.right, rect.bottom);
                    canvas.blitToD2D(s_d2dContext.Get());
                    
                    s_d2dContext->EndDraw();
                    s_skiaSwapChain->Present(1, 0);
                }
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
    wc.lpszClassName = "SetuMainWindow";
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
    wcGroup.lpszClassName = "SetuViewGroup";
    wcGroup.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent
    wcGroup.hCursor = LoadCursor(nullptr, IDC_ARROW);
    
    if (!RegisterClassEx(&wcGroup)) {
        Logger::e("WindowManager", "Failed to register view group window class!");
        return false;
    }
    
    s_mainWindow = CreateWindowEx(
        0,
        "SetuMainWindow",
        "Setu Runtime - Direct2D",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        540, 1170, // Average modern phone aspect ratio
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    s_skiaWindow = CreateWindowEx(
        0,
        "SetuMainWindow",
        "Setu Runtime - Skia",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        540, 1170, // Average modern phone aspect ratio
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!s_mainWindow || !s_skiaWindow) {
        Logger::e("WindowManager", "Failed to create main window!");
        return false;
    }
    
    if (!initDirect2D()) {
        Logger::e("WindowManager", "Failed to initialize Direct2D and DXGI!");
        return false;
    }
    
    SetTimer(s_mainWindow, TIMER_IDLE_GHOST, 600000, nullptr); // 10 minute idle timer

    // Position them nicely on screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int windowWidth = 540;
    int windowHeight = 1170;
    
    int startX = (screenWidth - (windowWidth * 2)) / 2;
    int startY = (screenHeight - windowHeight) / 2;
    if (startY < 0) startY = 0;
    
    SetWindowPos(s_mainWindow, nullptr, startX, startY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(s_skiaWindow, nullptr, startX + windowWidth, startY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(s_mainWindow, SW_SHOW);
    UpdateWindow(s_mainWindow);
    
    ShowWindow(s_skiaWindow, SW_SHOW);
    UpdateWindow(s_skiaWindow);
    
    Logger::i("WindowManager", "Initialized main window and Direct2D successfully.");
    return true;
}

void WindowManager::runMessageLoop() {
    setu::os::Looper::prepareMainLooper();
    Logger::i("WindowManager", "Starting message loop...");
    MSG msg;
    
    // Around the world, around the world...
    // Around the world, around the world...
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    Logger::i("WindowManager", "Message loop ended.");
}

HWND WindowManager::getMainWindow() {
    return s_mainWindow;
}

void WindowManager::wakeLooper(long long delayMs) {
    if (s_mainWindow) {
        if (delayMs <= 0) {
            PostMessage(s_mainWindow, WM_LOOPER_WAKE, 0, 0);
        } else {
            SetTimer(s_mainWindow, TIMER_LOOPER, delayMs, nullptr);
        }
    }
}

void WindowManager::pumpLooper() {
    setu::os::Looper* looper = setu::os::Looper::getMainLooper();
    if (looper) {
        while (looper->loopOnce()) {
            // Keep pumping while there are messages due
        }
    }
}
