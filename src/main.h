// main.h
#pragma once
#include <mutex>
#include <string>

class BasePath {
   public:
    static BasePath& instance();
    void set(const std::string& path);
    std::string get() const;
    std::string file(const std::string& fname) const;

   private:
    std::string base_path_;
    mutable std::mutex mutex_;
    BasePath();
    BasePath(const BasePath&) = delete;
    BasePath& operator=(const BasePath&) = delete;
};
