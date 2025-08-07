#include "base_path.h"

// Define BasePath default constructor
BasePath::BasePath() = default;

// Implementation of BasePath
BasePath &BasePath::instance() {
    static BasePath inst;
    return inst;
}
void BasePath::set(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex_);
    base_path_ = path;
    if (!base_path_.empty() && base_path_.back() != '/' && base_path_.back() != '\\') {
        base_path_ += "/";
    }
}
std::string BasePath::get() const { return base_path_; }
std::string BasePath::file(const std::string &fname) const { return base_path_ + fname; }
