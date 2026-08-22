#include <windows.h>
#include "WindowManager.h"

#include <iostream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    std::cout << "SetuLauncher started." << std::endl;

    if (!WindowManager::init(hInstance, nCmdShow)) {
        return 1;
    }
    
    return WindowManager::runMessageLoop();
}
