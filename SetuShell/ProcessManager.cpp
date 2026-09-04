#include "pch.h"
#include "ProcessManager.h"
#include <winrt/Windows.Data.Json.h>
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <chrono>
#include "utils/Logger.h"

namespace winrt::SetuShell::implementation
{
    struct EnumWindowsData {
        DWORD pid;
        HWND hwnd;
    };

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
        EnumWindowsData* data = reinterpret_cast<EnumWindowsData*>(lParam);
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        if (processId == data->pid) {
            // Check if window is visible and doesn't have an owner (top level)
            if (IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == NULL) {
                data->hwnd = hwnd;
                return FALSE; // Stop enumerating
            }
        }
        return TRUE;
    }

    ProcessManager& ProcessManager::Get() {
        static ProcessManager instance;
        return instance;
    }

    ProcessManager::ProcessManager() : m_running(true) {
        m_monitorThread = std::thread(&ProcessManager::MonitorThread, this);
    }

    ProcessManager::~ProcessManager() {
        m_running = false;
        if (m_monitorThread.joinable()) {
            m_monitorThread.join();
        }
        for (auto& pair : m_processes) {
            CloseHandle(pair.second.processHandle);
        }
    }

    void ProcessManager::LaunchApp(const std::wstring& package, const std::wstring& appPath) {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_processes.find(package) != m_processes.end()) {
            // Already running! Bring to front.
            // Note: EnumWindows + PID is a heuristic. It may fail if the window isn't created yet or if there are multiple top-level windows.
            EnumWindowsData data = { m_processes[package].pid, NULL };
            EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
            if (data.hwnd) {
                SetForegroundWindow(data.hwnd);
            } else {
                Logger::w("ProcessManager", "Could not find window for PID " + std::to_string(m_processes[package].pid) + " to foreground.");
            }
            return;
        }

        wchar_t modulePath[MAX_PATH];
        GetModuleFileNameW(NULL, modulePath, MAX_PATH);
        std::wstring moduleDir = modulePath;
        size_t lastSlash = moduleDir.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            moduleDir = moduleDir.substr(0, lastSlash);
        }
        std::wstring exePath = moduleDir + L"\\setu_runtime.exe";
        
        std::wstring safeAppPath = appPath;
        if (!safeAppPath.empty() && safeAppPath.back() == L'\\') {
            safeAppPath.pop_back();
        }

        // Convert paths for CLI
        std::wstring cmdLine = L"\"" + exePath + L"\" --app-path=\"" + safeAppPath + L"\" --package=\"" + package + L"\"";

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        std::vector<wchar_t> cmdLineMutable(cmdLine.begin(), cmdLine.end());
        cmdLineMutable.push_back(L'\0');

        if (CreateProcessW(exePath.c_str(), cmdLineMutable.data(), NULL, NULL, FALSE, 0, NULL, moduleDir.c_str(), &si, &pi)) {
            ProcessInfo info;
            info.processHandle = pi.hProcess;
            info.pid = pi.dwProcessId;
            info.launchTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            info.intentionallyTerminated = false;
            m_processes[package] = info;
            CloseHandle(pi.hThread);

            // Update manifest last_run_at
            PWSTR path = NULL;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path))) {
                std::wstring manifestPath = std::wstring(path) + L"\\Setu\\Apps\\" + package + L"\\manifest.json";
                CoTaskMemFree(path);
                
                std::ifstream inFile(manifestPath);
                if (inFile.is_open()) {
                    std::string jsonStr((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
                    inFile.close();
                    
                    winrt::Windows::Data::Json::JsonObject json = winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(jsonStr));
                    
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    char buf[64];
                    ctime_s(buf, sizeof(buf), &time);
                    std::string timeStr(buf);
                    if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();

                    json.SetNamedValue(L"last_run_at", winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(timeStr)));
                    
                    std::ofstream outFile(manifestPath);
                    if (outFile.is_open()) {
                        std::string outJson = winrt::to_string(json.Stringify());
                        outFile.write(outJson.c_str(), outJson.size());
                        outFile.close();
                    }
                }
            }
            Logger::i("ProcessManager", "Launched " + winrt::to_string(package) + " (PID: " + std::to_string(pi.dwProcessId) + ")");
        } else {
            Logger::e("ProcessManager", "Failed to launch " + winrt::to_string(package));
        }
    }

    void ProcessManager::ForceStop(const std::wstring& package) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_processes.find(package) != m_processes.end()) {
            m_processes[package].intentionallyTerminated = true;
            TerminateProcess(m_processes[package].processHandle, 0);
            Logger::i("ProcessManager", "Force stopped " + winrt::to_string(package));
        }
    }

    bool ProcessManager::IsAppRunning(const std::wstring& package) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_processes.find(package) != m_processes.end();
    }

    std::map<std::wstring, ProcessInfo> ProcessManager::GetRunningApps() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_processes;
    }

    int ProcessManager::AddProcessExitedHandler(ProcessExitedHandler handler) {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        int token = m_nextHandlerToken++;
        m_processExitedHandlers[token] = handler;
        return token;
    }

    void ProcessManager::RemoveProcessExitedHandler(int token) {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        m_processExitedHandlers.erase(token);
    }

    void ProcessManager::MonitorThread() {
        while (m_running) {
            std::vector<HANDLE> handles;
            std::vector<std::wstring> packages;
            
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (const auto& pair : m_processes) {
                    handles.push_back(pair.second.processHandle);
                    packages.push_back(pair.first);
                }
            }

            if (handles.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                continue;
            }

            // Wait for any process to exit or timeout after 500ms
            DWORD result = WaitForMultipleObjects(handles.size(), handles.data(), FALSE, 500);
            
            if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + handles.size()) {
                int index = result - WAIT_OBJECT_0;
                std::wstring package = packages[index];
                
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_processes.find(package);
                if (it != m_processes.end()) {
                    DWORD exitCode = 0;
                    GetExitCodeProcess(it->second.processHandle, &exitCode);
                    
                    if (it->second.intentionallyTerminated) {
                        Logger::i("ProcessManager", "Process for " + winrt::to_string(package) + " exited (Force Stopped).");
                    } else if (exitCode == 0) {
                        Logger::i("ProcessManager", "Process for " + winrt::to_string(package) + " exited normally.");
                    } else {
                        Logger::w("ProcessManager", "Process for " + winrt::to_string(package) + " exited with code " + std::to_string(exitCode));
                    }

                    CloseHandle(it->second.processHandle);
                    m_processes.erase(it);
                    
                    {
                        std::lock_guard<std::mutex> handlerLock(m_handlersMutex);
                        for (const auto& handlerPair : m_processExitedHandlers) {
                            handlerPair.second(package);
                        }
                    }
                }
            }
        }
    }
}