#pragma once

#include <string>
#include <vector>
#include <optional>
#include "coverage_mode.hpp"

namespace optimal_samples {

/**
 * @brief 算法参数配置结构
 */
struct AlgorithmParameters {
    // 基本参数
    CoverageMode mode;                    // 覆盖模式
    size_t maxSamples;                    // 最大样本数量
    double minCoverageRate;               // 最小覆盖率要求

    // Mode B 特定参数
    std::optional<size_t> minCoverage;    // 每个特征最小覆盖次数（Mode B）

    // Mode C 特定参数
    std::optional<size_t> combinationSize; // 特征组合大小（Mode C）

    // 优化参数
    size_t maxThreads{4};                 // 最大线程数
    size_t timeoutSeconds{300};           // 超时时间（秒）
    bool enableParallel{true};            // 是否启用并行计算

    // 构造函数
    AlgorithmParameters(
        CoverageMode mode,
        size_t maxSamples,
        double minCoverageRate
    ) : mode(mode),
        maxSamples(maxSamples),
        minCoverageRate(minCoverageRate) {}

    // 验证参数是否有效
    bool validate() const {
        // 基本参数验证
        if (maxSamples == 0 || minCoverageRate < 0 || minCoverageRate > 1) {
            return false;
        }

        // Mode B 参数验证
        if (mode == CoverageMode::MODE_B && !minCoverage.has_value()) {
            return false;
        }

        // Mode C 参数验证
        if (mode == CoverageMode::MODE_C && !combinationSize.has_value()) {
            return false;
        }

        return true;
    }
};

/**
 * @brief 算法结果结构
 */
struct AlgorithmResult {
    std::vector<Sample::ID> selectedSamples;  // 选中的样本ID列表
    double coverageRate;                      // 实际达到的覆盖率
    double executionTime;                     // 执行时间（秒）
    bool success;                             // 是否成功找到解
    std::string message;                      // 结果消息或错误信息

    AlgorithmResult(
        const std::vector<Sample::ID>& samples,
        double coverage,
        double time,
        bool success,
        const std::string& msg = ""
    ) : selectedSamples(samples),
        coverageRate(coverage),
        executionTime(time),
        success(success),
        message(msg) {}
};

} // namespace optimal_samples 