#pragma once

#include "types.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <chrono>
#include <string>
#include <memory>
#include <vector>

namespace core_algo {

class BaseSetCoverSolver {
protected:
    class Timer {
    private:
        std::string m_name;
        std::chrono::high_resolution_clock::time_point m_start;
    public:
        Timer(const std::string& name) : m_name(name), m_start(std::chrono::high_resolution_clock::now()) {}
        ~Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        }
        
        double getElapsedTime() const {
            auto end = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() / 1000.0;
        }
    };

public:
    explicit BaseSetCoverSolver(const Config& config) : m_config(config) {}
    virtual ~BaseSetCoverSolver() = default;

    // 主要求解函数
    virtual DetailedSolution solve(
        int universeSize,   // 总体大小
        int n,             // 从总体中选择的样本数量
        const std::vector<int>& samples,  // 输入样本
        int k,             // 每组样本数量
        int s,             // 需要覆盖的子集大小
        int j              // j参数
    ) = 0;

protected:
    // 验证解的正确性
    virtual bool verifySolution(
        const std::vector<int>& samples,
        int s,
        const Solution& solution
    ) const = 0;

    // 计算解的指标
    virtual std::vector<double> calculateMetrics(
        const std::vector<int>& samples,
        int s,
        const Solution& solution
    ) const = 0;

    // 构建覆盖矩阵
    virtual std::vector<std::vector<bool>> buildCoverageMatrix(
        const std::vector<std::vector<int>>& universe,
        const std::vector<std::vector<int>>& candidates
    ) = 0;

    // 选择下一个最优集合
    virtual size_t selectNextSet(
        const std::vector<std::vector<bool>>& coverageMatrix,
        const std::vector<bool>& isCovered,
        const std::vector<bool>& isSelected
    ) = 0;

    // 验证输入参数
    bool validateParameters(int universeSize, int n, int k, int s, int j) const {
        return (universeSize > 0 && n > 0 && k > 0 && s > 0 && j > 0 &&
                k >= s && j >= s && k <= n && s <= n && j <= n);
    }

    Config m_config;
};

} // namespace core_algo 