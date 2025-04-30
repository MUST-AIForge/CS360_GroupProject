#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>
#include <memory>
#include <ctime>
#include <iomanip>

namespace optimal_samples {
namespace utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::app);
    }

    void setLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        currentLevel_ = level;
    }

    template<typename... Args>
    void log(LogLevel level, const char* format, Args... args) {
        if (level < currentLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        std::string message = formatMessage(format, args...);
        std::string timestamp = getCurrentTimestamp();
        std::string levelStr = getLevelString(level);

        std::stringstream ss;
        ss << timestamp << " [" << levelStr << "] " << message << std::endl;

        if (logFile_.is_open()) {
            logFile_ << ss.str();
            logFile_.flush();
        }
        std::cout << ss.str();
    }

    template<typename... Args>
    void debug(const char* format, Args... args) {
        log(LogLevel::DEBUG, format, args...);
    }

    template<typename... Args>
    void info(const char* format, Args... args) {
        log(LogLevel::INFO, format, args...);
    }

    template<typename... Args>
    void warning(const char* format, Args... args) {
        log(LogLevel::WARNING, format, args...);
    }

    template<typename... Args>
    void error(const char* format, Args... args) {
        log(LogLevel::ERROR, format, args...);
    }

    template<typename... Args>
    void fatal(const char* format, Args... args) {
        log(LogLevel::FATAL, format, args...);
    }

private:
    Logger() : currentLevel_(LogLevel::INFO) {}
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    std::string getLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
            default:                return "UNKNOWN";
        }
    }

    template<typename T>
    std::string toString(const T& value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    template<typename... Args>
    std::string formatMessage(const char* format, Args... args) {
        size_t size = snprintf(nullptr, 0, format, args...) + 1;
        std::unique_ptr<char[]> buf(new char[size]);
        snprintf(buf.get(), size, format, args...);
        return std::string(buf.get(), buf.get() + size - 1);
    }

    std::mutex mutex_;
    std::ofstream logFile_;
    LogLevel currentLevel_;
};

// 便捷宏定义
#define LOG_DEBUG(...) optimal_samples::utils::Logger::getInstance().debug(__VA_ARGS__)
#define LOG_INFO(...) optimal_samples::utils::Logger::getInstance().info(__VA_ARGS__)
#define LOG_WARNING(...) optimal_samples::utils::Logger::getInstance().warning(__VA_ARGS__)
#define LOG_ERROR(...) optimal_samples::utils::Logger::getInstance().error(__VA_ARGS__)
#define LOG_FATAL(...) optimal_samples::utils::Logger::getInstance().fatal(__VA_ARGS__)

} // namespace utils
} // namespace optimal_samples 