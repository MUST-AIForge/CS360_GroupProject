#pragma once

#include <string>
#include <chrono>
#include <iostream>

namespace core_algo {

class Timer {
public:
    explicit Timer(const std::string& name) 
        : name_(name), start_(std::chrono::high_resolution_clock::now()) {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        std::cout << name_ << " 耗时: " << duration.count() / 1000.0 << "ms" << std::endl;
    }

    double elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        return duration.count() / 1000.0;  // 返回毫秒
    }

private:
    std::string name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

} // namespace core_algo 