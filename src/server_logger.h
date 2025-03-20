#pragma once
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

class ServerLogger {
    std::ofstream log_file;
    std::string file_name;
    bool verbose;
    int retries = 0;
    const int MAX_RETRIES = 3;

    const size_t MAX_LOG_SIZE = 10 * 1024 * 1024;  // 10MB size limit (adjustable)
    const size_t BUFFER_LIMIT = 0;                 // 4KB buffer limit before flushing

    std::ostringstream buffer;

    std::string cached_time;
    std::time_t last_time_t = 0;
    std::chrono::steady_clock::time_point last_time_check = std::chrono::steady_clock::now();
    const std::chrono::seconds TIME_UPDATE_INTERVAL = std::chrono::seconds(1);

    // Get cached time, updated once per second
    std::string get_cached_time() {
        auto now = std::chrono::steady_clock::now();
        if (now - last_time_check > TIME_UPDATE_INTERVAL) {
            last_time_check = now;
            auto now_time_t =
                std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm* now_tm = std::localtime(&now_time_t);
            std::ostringstream oss;
            oss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
            cached_time = oss.str();
        }
        return cached_time;
    }

    // Flush buffer content to file
    void flush_buffer() {
        if (log_file.is_open() && !buffer.str().empty()) {
            log_file << buffer.str();
            log_file.flush();  // Ensure data is written to disk
            buffer.str("");
            buffer.clear();
        }
    }

    // Attempt to reopen log file if it's closed
    bool reopen_file() {
        if (retries >= MAX_RETRIES) {
            return false;  // Stop retrying after reaching the limit
        }
        log_file.close();                         // Close if half-open
        log_file.open(file_name, std::ios::app);  // Reopen in append mode
        if (!log_file.is_open()) {
            ++retries;
            std::cerr << "Failed to reopen log file: " << file_name << " (Attempt " << retries
                      << "/" << MAX_RETRIES << ")" << std::endl;
            return false;
        }
        retries = 0;  // Reset retry count on success
        return true;
    }

    // Internal file logging with retries, size check, and buffer flushing
    template <typename... Args>
    void log_to_file(const Args&... args) {
        if (!log_file.is_open() && !reopen_file()) {
            return;  // Give up if cannot reopen
        }

        buffer << get_cached_time() << ": ";
        (buffer << ... << args) << '\n';

        if (buffer.tellp() >= static_cast<std::streampos>(BUFFER_LIMIT)) {
            flush_buffer();  // Flush when buffer exceeds limit
        }
    }

   public:
    // Public log method for debug-level logs
    template <typename... Args>
    void debug_log(const Args&... args) {
        if (verbose) {
            log_to_file(args...);
        }
    }

    template <typename... Args>
    void error_log(const Args&... args) {
        log_to_file(args...);
    }

    // Constructor with file open
    ServerLogger(bool verbose, const std::string& file_name)
        : verbose(verbose), file_name(file_name) {
        std::filesystem::path path(file_name);
        std::filesystem::create_directories(path.parent_path());
        log_file.open(file_name, std::ios::app);  // Open in append mode
        if (!log_file.is_open()) {
            std::cerr << "Error opening log file: " << file_name << std::endl;
        }
    }

    // Destructor flushes buffer and closes file
    ~ServerLogger() {
        flush_buffer();  // Ensure nothing is left in buffer
        if (log_file.is_open()) {
            log_file.close();
        }
    }
};
