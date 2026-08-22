#include "WindowManager.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <windowsx.h>

#include <filesystem>

HWND WindowManager::s_hwnd = nullptr;
std::vector<InstalledApp> WindowManager::s_installedApps;

void WindowManager::discoverApps() {
    s_installedApps.clear();
    const char* localAppData = getenv("LOCALAPPDATA");
    if (!localAppData) return;
    
    std::string appsDir = std::string(localAppData) + "\\Setu\\apps";
    std::error_code ec;
    if (!std::filesystem::exists(appsDir, ec)) {
        std::filesystem::create_directories(appsDir, ec);
        std::cout << "No apps installed yet" << std::endl;
        return;
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(appsDir, ec)) {
        if (entry.is_directory()) {
            InstalledApp app;
            app.packageName = entry.path().filename().string();
            app.displayName = app.packageName; // fallback
            s_installedApps.push_back(app);
        }
    }
    
    if (s_installedApps.empty()) {
        std::cout << "No apps installed yet" << std::endl;
    } else {
        std::cout << "Discovered apps:" << std::endl;
        for (size_t i = 0; i < s_installedApps.size() && i < 9; ++i) {
            std::cout << (i + 1) << ". " << s_installedApps[i].displayName << std::endl;
        }
    }
}

void WindowManager::spawnRuntime(const std::string& packageName) {
    
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    std::string path(exePath);
    size_t lastSlash = path.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        path = path.substr(0, lastSlash);
    }
    std::string runtimePath = path + "\\..\\..\\..\\build\\Debug\\setu_runtime.exe";
    
    std::string cmdLine = "\"" + runtimePath + "\"";
    if (!packageName.empty()) {
        cmdLine += " --package=" + packageName;
    }
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    char cmdBuffer[1024];
    strcpy_s(cmdBuffer, cmdLine.c_str());
    
    if (CreateProcessA(NULL, cmdBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cout << "Spawned setu_runtime.exe (PID=" << pi.dwProcessId << ") for package=" << (packageName.empty() ? "(none)" : packageName) << std::endl;
        
        std::thread([hProcess = pi.hProcess, pid = pi.dwProcessId, packageName]() {
            waitAndLogProcess(hProcess, pid, packageName);
        }).detach();
        
        CloseHandle(pi.hThread);
    } else {
        std::cout << "Failed to spawn setu_runtime.exe. Error: " << GetLastError() << std::endl;
    }
}

void WindowManager::waitAndLogProcess(HANDLE hProcess, DWORD pid, std::string packageName) {
    WaitForSingleObject(hProcess, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeProcess(hProcess, &exitCode);
    
    std::cout << "setu_runtime.exe (PID=" << pid << ") exited with code " << exitCode << std::endl;
    
    if (exitCode != 0 && !packageName.empty()) {
        const char* localAppData = getenv("LOCALAPPDATA");
        if (localAppData) {
            std::string crashLogPath = std::string(localAppData) + "\\Setu\\apps\\" + packageName + "\\crash.log";
            std::ifstream ifs(crashLogPath);
            if (ifs.is_open()) {
                std::cout << "[CRASH LOG]\n";
                std::string line;
                while (std::getline(ifs, line)) {
                    std::cout << line << std::endl;
                }
            }
        }
    }
    
    CloseHandle(hProcess);
}

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
    
    discoverApps();
    
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
        case WM_LBUTTONDOWN: {
            int xPos = GET_X_LPARAM(lParam); 
            int yPos = GET_Y_LPARAM(lParam); 
            RECT btnRect = { 300, 250, 500, 300 };
            if (xPos >= btnRect.left && xPos <= btnRect.right && yPos >= btnRect.top && yPos <= btnRect.bottom) {
                spawnRuntime("");
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            
            RECT btnRect = { 300, 250, 500, 300 };
            HBRUSH btnBrush = CreateSolidBrush(RGB(200, 200, 200));
            FillRect(hdc, &btnRect, btnBrush);
            DeleteObject(btnBrush);
            
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, "Launch Runtime", -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
