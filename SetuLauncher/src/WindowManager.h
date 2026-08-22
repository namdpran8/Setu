#pragma once
#include <windows.h>

class WindowManager {
public:
    static bool init(HINSTANCE hInstance, int nCmdShow);
    static int runMessageLoop();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HWND s_hwnd;
};
