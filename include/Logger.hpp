// Logger.hpp
/** Usage:
    Logger::Log(LogLevel::Debug, "Generating terrain");
    Logger::Log(LogLevel::Info, "Terrain generation completed");
    Logger::Log(LogLevel::Warning, "...");
    Logger::Log(LogLevel::Error, "...");
*/
#pragma once

#include <fstream>
#include <ios>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <source_location>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static void Init(const std::string& filepath);
    static void Shutdown();

    static void Log(LogLevel level, const std::string& message, std::source_location location = std::source_location::current());
    
    // Concatenates any streamable arguments (int, float, double, bool, char,
    // const char*, std::string, ...) into a single string.
    template<typename... Args>
    static std::string Concat(Args&&... args) {
        std::ostringstream oss;
        oss << std::boolalpha; // true/false instead of 1/0
        (oss << ... << std::forward<Args>(args));
        return oss.str();
    }

private:
    static std::ofstream file;
    static std::mutex mutex;
};

// Debug logging is removed in Release builds.
#ifdef DEBUG_MODE
    #define LOG_DEBUG(...) \
        do { Logger::Log(LogLevel::Debug, __VA_ARGS__); } while (false)
#else
    #define LOG_DEBUG(...) do { } while (false)
#endif

// These remain available in Release builds.
#define LOG_INFO(...) \
    do { Logger::Log(LogLevel::Info, __VA_ARGS__); } while (false)

#define LOG_WARNING(...) \
    do { Logger::Log(LogLevel::Warning, __VA_ARGS__); } while (false)

#define LOG_ERROR(...) \
    do { Logger::Log(LogLevel::Error, __VA_ARGS__); } while (false)