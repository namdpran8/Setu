#pragma once
#include <windows.h>
#include <string>
#include <functional>

class WindowManager {
public:
    static bool init();
    static void runMessageLoop();
    static HWND getMainWindow();

    // UI creation helpers
    static HWND createStaticText(const std::string& text, int x, int y, int width, int height);
    static void clearWindow();

    static void setClickCallback(std::function<void(int)> cb);

private:
    static std::function<void(int)> s_clickCallback;
    static HWND s_mainWindow;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
