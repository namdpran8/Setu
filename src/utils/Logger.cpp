#include "Logger.h"
#include <iostream>

// Default log level is DEBUG so we see everything by default
LogLevel Logger::s_currentLevel = LogLevel::DEBUG;

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
    *stream << levelStr << " " << tag << ": " << message << std::endl;
}
