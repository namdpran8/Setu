#pragma once
#include <string>
#include <map>
#include <windows.h>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <winrt/Windows.Foundation.h>

namespace winrt::SetuShell::implementation
{
    struct ProcessInfo {
        HANDLE processHandle;
        DWORD pid;
        int64_t launchTimestamp;
        bool intentionallyTerminated;
    };

    class ProcessManager {
    public:
        static ProcessManager& Get();

        void LaunchApp(const std::wstring& package, const std::wstring& appPath);
        void ForceStop(const std::wstring& package);
        bool IsAppRunning(const std::wstring& package);
        
        std::map<std::wstring, ProcessInfo> GetRunningApps();
        
        // Fired when a process exits
        using ProcessExitedHandler = std::function<void(const std::wstring&)>;
        int AddProcessExitedHandler(ProcessExitedHandler handler);
        void RemoveProcessExitedHandler(int token);

    private:
        ProcessManager();
        ~ProcessManager();

        void MonitorThread();

        std::map<std::wstring, ProcessInfo> m_processes;
        std::mutex m_mutex;
        std::thread m_monitorThread;
        std::atomic<bool> m_running;

        std::map<int, ProcessExitedHandler> m_processExitedHandlers;
        int m_nextHandlerToken = 1;
        std::mutex m_handlersMutex;
    };
}