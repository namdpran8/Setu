#pragma once
#include <windows.h>

#include <string>

#include <vector>

struct InstalledApp {
    std::string packageName;
    std::string displayName;
};

class WindowManager {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);
    static int runMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HWND s_hwnd;
    
    static std::vector<InstalledApp> s_installedApps;
    static void discoverApps();
    static void spawnRuntime(const std::string& packageName = "");
    static void waitAndLogProcess(HANDLE hProcess, DWORD pid, std::string packageName);
};
