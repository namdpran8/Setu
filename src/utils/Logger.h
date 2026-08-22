#pragma once
#include <string>

// We define our log levels. 
// Note: We use 'ERR' instead of 'ERROR' because Windows headers 
// sometimes define 'ERROR' as a macro, which causes conflicts!
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERR
};

#include <fstream>

class Logger {
public:
    // Android-style static logging methods
    static void d(const std::string& tag, const std::string& message); // Debug
    static void i(const std::string& tag, const std::string& message); // Info
    static void w(const std::string& tag, const std::string& message); // Warning
    static void e(const std::string& tag, const std::string& message); // Error
    
    // Set the minimum level to print (e.g., set to INFO to hide DEBUG logs)
    static void setLevel(LogLevel level);

    static void setConfiguredLevel(const std::string& level);

private:
    static std::string s_configuredLevel;
    static LogLevel s_currentLevel;
    static std::ofstream s_logFile;
    static void initLogFile();
    static void log(LogLevel level, const std::string& tag, const std::string& message);
};
