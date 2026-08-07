#include "WindowManager.h"
#include "../utils/Logger.h"

HWND WindowManager::s_mainWindow = nullptr;

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
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
        Logger::e("WindowManager", "Failed to register window class!");
        return false;
    }
    
    s_mainWindow = CreateWindowEx(
        0,
        "WindroidMainWindow",
        "Windroid Runtime",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        400, 800, // Roughly a phone portrait aspect ratio
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    
    if (!s_mainWindow) {
        Logger::e("WindowManager", "Failed to create main window!");
        return false;
    }
    
    // We do NOT show the window here. We let the activity onCreate() call ShowWindow!
    Logger::i("WindowManager", "Initialized main window successfully.");
    
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

HWND WindowManager::createStaticText(const std::string& text, int x, int y, int width, int height) {
    if (!s_mainWindow) return nullptr;
    
    HWND hStatic = CreateWindowEx(
        0,
        "STATIC",
        text.c_str(),
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        x, y, width, height,
        s_mainWindow,
        nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );
    
    return hStatic;
}
