#include "WindowManager.h"
#include "../utils/Logger.h"

HWND WindowManager::s_mainWindow = nullptr;
std::function<void(int)> WindowManager::s_clickCallback = nullptr;

void WindowManager::setClickCallback(std::function<void(int)> cb) {
    s_clickCallback = cb;
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

LRESULT CALLBACK WindowManager::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                HWND controlHwnd = (HWND)lParam;
                // LOWORD(wParam) truncates 32-bit Android IDs to 16 bits!
                // We MUST get the full 32-bit ID from the HWND itself.
                int controlId = (int)GetWindowLongPtr(controlHwnd, GWLP_ID);
                Logger::d("WindowManager", "Button clicked with ID: " + std::to_string(controlId));
                if (s_clickCallback) {
                    s_clickCallback(controlId);
                }
            }
            return 0;
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
