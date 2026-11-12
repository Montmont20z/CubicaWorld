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
        (oss << ... << std::forward<Args>(args)); // Concat("x=", 5, " z=", 3.5) becomes: (((oss << "x=") << 5) << " z=") << 3.5; // a loop that runs at compile time, so you don’t need recursion
        return oss.str();
    }

private:
    static std::ofstream file;
    static std::mutex mutex;
};

// The macros capture source_location at the call site, which is why they
// (not a variadic Log function) do the forwarding.
#define LOG_IMPL(level, ...) \
    do { Logger::Log(level, Logger::Concat(__VA_ARGS__), std::source_location::current()); } while(false)

// Debug logging is removed in Release builds.
#ifdef DEBUG_MODE
    #define LOG_DEBUG(...) LOG_IMPL(LogLevel::Debug, __VA_ARGS__)
#else
    #define LOG_DEBUG(...) do { } while (false)
#endif

// These remain available in Release builds.
#define LOG_INFO(...) LOG_IMPL(LogLevel::Info, __VA_ARGS__)
#define LOG_WARNING(...) LOG_IMPL(LogLevel::Warning, __VA_ARGS__)
#define LOG_ERROR(...) LOG_IMPL(LogLevel::Error, __VA_ARGS__)