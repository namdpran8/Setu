#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

// Default log level is DEBUG so we see everything by default
LogLevel Logger::s_currentLevel = LogLevel::DEBUG;
std::ofstream Logger::s_logFile;

void Logger::initLogFile() {
    std::error_code ec;
    std::filesystem::create_directory("logs", ec);
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "logs/log_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".txt";
    s_logFile.open(ss.str(), std::ios::out | std::ios::app);

    if (!s_logFile.is_open()) {
        // Fallback to current directory
        std::stringstream ssFallback;
        ssFallback << "log_" << std::put_time(std::localtime(&time), "%Y%m%d_%H%M%S") << ".txt";
        s_logFile.open(ssFallback.str(), std::ios::out | std::ios::app);
    }
    
    if (!s_logFile.is_open()) {
        // Absolute fallback
        s_logFile.open("run_log.txt", std::ios::out | std::ios::app);
    }
}

void Logger::setLevel(LogLevel level) {
    s_currentLevel = level;
}

void Logger::d(const std::string& tag, const std::string& message) {
    log(LogLevel::DEBUG, tag, message);
}

void Logger::i(const std::string& tag, const std::string& message) {
    log(LogLevel::INFO, tag, message);
}

void Logger::w(const std::string& tag, const std::string& message) {
    log(LogLevel::WARN, tag, message);
}

void Logger::e(const std::string& tag, const std::string& message) {
    log(LogLevel::ERR, tag, message);
}

void Logger::log(LogLevel level, const std::string& tag, const std::string& message) {
    if (level < s_currentLevel) {
        return; // Skip logging if the level is too low
    }

    // Pick a prefix and stream based on the level
    std::string levelStr;
    std::ostream* stream = &std::cout;

    switch (level) {
        case LogLevel::DEBUG: levelStr = "[DEBUG]"; break;
        case LogLevel::INFO:  levelStr = "[INFO] "; break;
        case LogLevel::WARN:  levelStr = "[WARN] "; break;
        case LogLevel::ERR:   
            levelStr = "[ERROR]"; 
            stream = &std::cerr; 
            break;
    }

    // Format: [INFO] ApkExtractor: Successfully opened APK.
    std::string fullMessage = levelStr + " " + tag + ": " + message;
    *stream << fullMessage << std::endl;
    
    static bool s_logInitFailed = false;
    if (!s_logFile.is_open() && !s_logInitFailed) {
        initLogFile();
        if (!s_logFile.is_open()) {
            s_logInitFailed = true; // Don't keep retrying if we can't create the file
        }
    }
    if (s_logFile.is_open()) {
        s_logFile << fullMessage << std::endl;
        s_logFile.flush();
    }
}
