#include "Logger.hpp"

std::ofstream Logger::file;
std::mutex Logger::mutex;

void Logger::Init(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);

    file.open(filepath, std::ios::out | std::ios::app);

    if (!file.is_open()) {
        std::cerr << "Failed to open log file: " << filepath << '\n';
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex);

    if (file.is_open()) {
        file.close();
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);

    const char* label = "";

    switch (level) {
        case LogLevel::Debug:   label = "DEBUG"; break;
        case LogLevel::Info:    label = "INFO"; break;
        case LogLevel::Warning: label = "WARNING"; break;
        case LogLevel::Error:   label = "ERROR"; break;
    }

    std::ostream& output =
        (level == LogLevel::Error) ? std::cerr : std::cout;

    output << '[' << label << "] " << message << '\n';

    if (file.is_open()) {
        file << '[' << label << "] " << message << '\n';
    }
}