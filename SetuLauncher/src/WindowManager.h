/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
