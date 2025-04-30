#pragma once

#include <vector>
#include <string>
#include <stdexcept>

namespace core_algo {

// 覆盖模式枚举
enum class CoverageMode {
    CoverMinOneS,    // Mode A: 至少覆盖一个s
    CoverMinNS,      // Mode B: 至少覆盖n个s
    CoverAllS,       // Mode C: 覆盖所有s
    UNION,           // 并集覆盖模式
    INTERSECTION,    // 交集覆盖模式
    AUTO             // 自动模式：根据n参数自动选择合适的模式
};

// 结果结构体（从 sample_selector.hpp 移动到这里）
struct Solution {
    std::vector<std::vector<int>> groups;  // 结果组集合
    int totalGroups;                       // 组数量
    double computationTime;                // 计算耗时
    bool isOptimal;                        // 是否为最优解
};

// 错误类型定义
class AlgorithmError : public std::runtime_error {
public:
    explicit AlgorithmError(const std::string& message) 
        : std::runtime_error(message) {}
};

// 配置结构体
struct Config {
    bool enableParallel = false;      // 是否启用并行计算
    int threadCount = 1;              // 并行线程数
    double timeLimit = 0.0;           // 时间限制（秒），0表示无限制
    bool enableCache = true;          // 是否启用缓存
    size_t maxCacheSize = 1000000;    // 最大缓存大小

    // 参数范围配置
    struct ParameterRanges {
        // 题目要求的默认范围
        int minM = 45;       // 最小总样本数量
        int maxM = 54;       // 最大总样本数量
        int minN = 7;        // 最小选择样本数量
        int maxN = 25;       // 最大选择样本数量
        int minK = 4;        // 最小组大小
        int maxK = 7;        // 最大组大小
        int minS = 3;        // 最小子集大小
        int maxS = 7;        // 最大子集大小
        int minCoverCount = 1;  // 最小覆盖数量
        int maxCoverCount = INT_MAX;  // 最大覆盖数量
    } ranges;

    // 验证参数是否在有效范围内
    bool validateParameters(int m, int n, int k, int s, int coverCount = 1) const {
        return m >= ranges.minM && m <= ranges.maxM &&
               n >= ranges.minN && n <= ranges.maxN &&
               k >= ranges.minK && k <= ranges.maxK &&
               s >= ranges.minS && s <= ranges.maxS &&
               coverCount >= ranges.minCoverCount && coverCount <= ranges.maxCoverCount &&
               k <= n && s <= k;  // 逻辑约束
    }
};

// 算法状态枚举
enum class Status {
    Success,
    Timeout,
    NoSolution,
    Error
};

// 详细结果结构体
struct DetailedSolution : public Solution {
    Status status;                    // 算法执行状态
    std::string message;              // 详细信息
    double coverageRatio;            // 覆盖率
    std::vector<double> metrics;     // 其他性能指标
};

} // namespace core_algo 