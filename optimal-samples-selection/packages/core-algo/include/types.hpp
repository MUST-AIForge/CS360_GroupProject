#pragma once

#include <vector>
#include <string>
#include <stdexcept>
#include <cmath>

namespace core_algo {

// 组合缓存结构体
struct CombinationCache {
    std::vector<std::vector<std::vector<int>>> jGroupSSubsets;  // 每个j组合的s子集集合
    std::vector<std::vector<int>> allSSubsets;                  // 所有s子集的扁平列表
};

// 覆盖模式枚举
enum class CoverageMode {
    CoverMinOneS,    // Mode A: 至少覆盖一个s
    CoverMinNS,      // Mode B: 至少覆盖n个s
    CoverAllS        // Mode C: 覆盖所有s
};

// 覆盖结果结构体
struct CoverageResult {
    double coverage_ratio;                 // 覆盖率
    int covered_j_count;                  // 被覆盖的j组合数量
    int total_j_count;                    // 总j组合数量
    std::vector<bool> j_coverage_status;  // 每个j组合的覆盖状态
    std::vector<int> j_covered_s_counts;  // 每个j组合中被覆盖的s子集数量
    int total_groups;                     // 总组数

    CoverageResult() : coverage_ratio(0.0), covered_j_count(0), 
                      total_j_count(0), total_groups(0) {}

    CoverageResult(double ratio, int covered_count, int total_count,
                  std::vector<bool> coverage_status,
                  std::vector<int> covered_s_counts,
                  int groups = 0)
        : coverage_ratio(ratio),
          covered_j_count(covered_count),
          total_j_count(total_count),
          j_coverage_status(std::move(coverage_status)),
          j_covered_s_counts(std::move(covered_s_counts)),
          total_groups(groups) {}
};

// 结果结构体
struct Solution {
    std::vector<std::vector<int>> groups;  // 结果组集合
    int totalGroups;                       // 组数量
    double computationTime;                // 计算耗时
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
    std::vector<int> inputSamples;   // 输入的样本集合，如果为空则生成随机样本
    bool enableCache = false;         // 是否启用缓存
    size_t maxCacheSize = 1000;      // 最大缓存大小
    bool enableRandomization = false; // 是否启用随机化
    int randomSeed = 0;              // 随机种子，0表示使用随机设备
    bool useLetter = false;          // 是否使用字母表示样本
    int n;  // 总样本数
    int j;  // j组大小
    int s;  // s子集大小
    int min_coverage_count;  // Mode B的最小覆盖次数要求
    size_t max_groups = 100;  // 最大组数限制

    // 参数范围配置
    struct ParameterRanges {
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

    Config() : n(0), j(0), s(0), min_coverage_count(1) {}
    Config(int n, int j, int s, int min_coverage_count = 1)
        : n(n), j(j), s(s), min_coverage_count(min_coverage_count) {}
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
    bool isOptimal = false;          // 是否为最优解

    bool operator==(const DetailedSolution& other) const {
        return status == other.status &&
               groups == other.groups &&
               std::abs(coverageRatio - other.coverageRatio) < 1e-6 &&
               totalGroups == other.totalGroups &&
               std::abs(computationTime - other.computationTime) < 1e-6 &&
               message == other.message &&
               isOptimal == other.isOptimal;
    }
};

} // namespace core_algo 