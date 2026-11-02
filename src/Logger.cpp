#include "Logger.hpp"
#include <filesystem>

std::ofstream Logger::file;
std::mutex Logger::mutex;

void Logger::Init(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::filesystem::path path(filepath);

    // create parent directories if they don't exist
    if (path.has_parent_path()){
        std::filesystem::create_directories(path.parent_path());
    }

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

void Logger::Log(LogLevel level, const std::string& message, std::source_location location) {
    std::lock_guard<std::mutex> lock(mutex);

    const char* label = "";

    switch (level) {
        case LogLevel::Debug:   label = "DEBUG"; break;
        case LogLevel::Info:    label = "INFO"; break;
        case LogLevel::Warning: label = "WARNING"; break;
        case LogLevel::Error:   label = "ERROR"; break;
    }
    
    auto write = [&](std::ostream& stream) {
        stream << '[' << label << "] "
               << '[' << location.file_name()
               << ':' << location.line() << "] "
               << '[' << location.function_name() << "] "
               << message << '\n';
    };
    
    std::ostream& output = (level == LogLevel::Error) ? std::cerr : std::cout;

    write(output);

    if (file.is_open()) {
        write(file);
    }
}