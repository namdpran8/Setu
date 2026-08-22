#include <windows.h>
#include "WindowManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!WindowManager::init(hInstance, nCmdShow)) {
        return 1;
    }
    
    return WindowManager::runMessageLoop();
}
