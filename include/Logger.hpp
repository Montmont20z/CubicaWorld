// Logger.hpp
/** Usage:
    Logger::Log(LogLevel::Debug, "Generating terrain");
    Logger::Log(LogLevel::Info, "Terrain generation completed");
*/
#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <string>

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

    static void Log(LogLevel level, const std::string& message);

private:
    static std::ofstream file;
    static std::mutex mutex;
};