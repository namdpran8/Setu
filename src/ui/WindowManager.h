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

#pragma once
#include <windows.h>

constexpr UINT WM_LOOPER_WAKE = WM_USER + 1;
constexpr UINT_PTR TIMER_LOOPER = 3;

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

    static void wakeLooper(long long delayMs);
    static void pumpLooper();

    static HWND getMainWindow();

    // D2D getters
    static ID2D1DeviceContext* getD2DContext();
    static IDWriteFactory* getDWriteFactory();
    static IDXGISwapChain1* getSwapChain();



    static void clearWindow();
    static void setWindowIcon(const std::string& iconPath);
    static void cleanupIcon();
    static void setClickCallback(std::function<void(int)> cb);
    static void setLongClickCallback(std::function<bool(int)> cb);
    static std::function<bool(int)> s_longClickCallback;
    static bool triggerLongClickCallback(int controlId);
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
    static HWND s_skiaWindow;
    static HICON s_customIconSmall;
    static HICON s_customIconBig;

    // Direct2D / DirectX resources
    static Microsoft::WRL::ComPtr<ID3D11Device> s_d3dDevice;
    static Microsoft::WRL::ComPtr<ID3D11DeviceContext> s_d3dContext;
    
    static Microsoft::WRL::ComPtr<IDXGISwapChain1> s_swapChain;
    static Microsoft::WRL::ComPtr<ID2D1Bitmap1> s_d2dTargetBitmap;
    
    static Microsoft::WRL::ComPtr<IDXGISwapChain1> s_skiaSwapChain;
    static Microsoft::WRL::ComPtr<ID2D1Bitmap1> s_skiaTargetBitmap;
    
    static Microsoft::WRL::ComPtr<ID2D1Factory1> s_d2dFactory;
    static Microsoft::WRL::ComPtr<ID2D1Device> s_d2dDevice;
    static Microsoft::WRL::ComPtr<ID2D1DeviceContext> s_d2dContext;
    static Microsoft::WRL::ComPtr<IDWriteFactory> s_dWriteFactory;

    static bool initDirect2D();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
