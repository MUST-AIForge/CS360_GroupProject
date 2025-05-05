#pragma once

#include "types.hpp"
#include <memory>
#include <vector>
#include <chrono>
#include <map>

namespace core_algo {

// 前向声明
class CombinationGenerator;
class SetOperations;
class CoverageCalculator;

class BaseSolver {
protected:
    Config m_config;
    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<CoverageCalculator> m_covCalc;

    // 基础数据
    std::vector<std::vector<int>> jGroups_;
    std::vector<std::vector<int>> candidates_;
    std::vector<std::vector<int>> selectedGroups_;
    
    // 映射关系
    std::map<std::vector<int>, std::vector<std::vector<int>>> jToSMap_;    // j组到其s子集的映射
    std::map<std::vector<int>, std::vector<std::vector<int>>> sToJMap_;    // s子集到包含它的j组的映射
    std::vector<std::vector<int>> allSSubsets_;                            // 所有可能的s子集

    // 组合生成结果结构
    struct CombinationResult {
        std::vector<std::vector<int>> groups;           // k组候选集
        std::vector<std::vector<int>> jCombinations;    // j组集合
        std::vector<std::vector<int>> allSSubsets;      // 所有s子集
        std::map<std::vector<int>, std::vector<std::vector<int>>> jToSMap; // j到s的映射
        std::map<std::vector<int>, std::vector<std::vector<int>>> sToJMap; // s到j的映射
    };

    // 生成组合并建立映射关系
    virtual CombinationResult generateCombinations(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j
    ) = 0;

public:
    explicit BaseSolver(const Config& config) : m_config(config) {}
    virtual ~BaseSolver() = default;

    // 验证解决方案
    CoverageResult validateSolution(
        const std::vector<std::vector<int>>& groups,
        const std::vector<int>& samples,
        int j,
        int s
    ) const;

    // 准备解决方案
    DetailedSolution prepareSolution(
        const std::vector<std::vector<int>>& selectedGroups,
        const CoverageResult& coverageResult,
        const std::chrono::steady_clock::time_point& startTime
    ) const;

    // 生成组合缓存
    CombinationCache generateCombinations(
        const std::vector<int>& samples,
        int j,
        int s
    ) const;

    // 纯虚函数：求解问题
    virtual DetailedSolution solve(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j
    ) = 0;
    
};

} // namespace core_algo 