#pragma once
#include <windows.h>
#include <string>

class WindowManager {
public:
    static bool init();
    static void runMessageLoop();
    static HWND getMainWindow();

    // UI creation helpers
    static HWND createStaticText(const std::string& text, int x, int y, int width, int height);

private:
    static HWND s_mainWindow;
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};
