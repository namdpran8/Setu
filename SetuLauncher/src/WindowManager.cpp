#include "WindowManager.h"

HWND WindowManager::s_hwnd = nullptr;

bool WindowManager::init(HINSTANCE hInstance, int nCmdShow) {
    const char CLASS_NAME[] = "SetuLauncherWindowClass";
    
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowManager::WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    RegisterClass(&wc);
    
    s_hwnd = CreateWindowEx(
        0, CLASS_NAME, "Setu Launcher",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, hInstance, nullptr
    );
    
    if (s_hwnd == nullptr) {
        return false;
    }
    
    ShowWindow(s_hwnd, nCmdShow);
    UpdateWindow(s_hwnd);
    
    return true;
}

int WindowManager::runMessageLoop() {
    MSG msg = { };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
