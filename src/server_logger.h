#pragma once
#include <chrono>
#include <iomanip>
#include <ctime>
#include <iostream>

class ServerLogger {
    bool verbose;
public:

    template<typename... Args>
    void debug_log(const Args&... args) {
        if (verbose) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
            std::tm* now_tm = std::localtime(&now_time_t);

            std::cout << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S") << ": ";

            (std::cout << ... << args) << std::endl;
        }
    }
    ServerLogger(bool verbose);
};
