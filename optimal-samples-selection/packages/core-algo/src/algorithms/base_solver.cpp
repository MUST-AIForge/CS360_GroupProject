#include "base_solver.hpp"
#include "set_operations.hpp"
#include "combination_generator.hpp"
#include "coverage_calculator.hpp"
#include <chrono>
#include <algorithm>
#include <map>
#include <set>

namespace core_algo {

DetailedSolution BaseSolver::solve(
    int m,
    int n,
    const std::vector<int>& samples,
    int k,
    int s,
    int j
) {
    // 基类的默认实现
    return DetailedSolution();
}

CoverageResult BaseSolver::validateSolution(
    const std::vector<std::vector<int>>& groups,
    const std::vector<int>& samples,
    int j,
    int s
) const {
    auto jCombinations = m_combGen->generate(samples, j);
    
    // 为每个j组合生成s子集
    std::vector<std::vector<std::vector<int>>> jGroupSSubsets;
    jGroupSSubsets.resize(jCombinations.size());
    for (size_t i = 0; i < jCombinations.size(); ++i) {
        jGroupSSubsets[i] = m_combGen->generate(jCombinations[i], s);
    }
    
    return m_covCalc->calculateCoverage(groups, jCombinations, jGroupSSubsets, CoverageMode::CoverMinOneS, 1);
}

DetailedSolution BaseSolver::prepareSolution(
    const std::vector<std::vector<int>>& selectedGroups,
    const CoverageResult& coverageResult,
    const std::chrono::steady_clock::time_point& startTime
) const {
    DetailedSolution solution;
    solution.groups = selectedGroups;
    solution.status = coverageResult.coverage_ratio >= 0.95 ? Status::Success : Status::Error;
    solution.coverageRatio = coverageResult.coverage_ratio;
    solution.totalGroups = selectedGroups.size();
    
    auto endTime = std::chrono::steady_clock::now();
    solution.computationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count() / 1000.0;
    
    return solution;
}

CombinationCache BaseSolver::generateCombinations(
    const std::vector<int>& samples,
    int j,
    int s
) const {
    CombinationCache cache;
    
    // 生成j组合
    cache.jCombinations = m_combGen->generate(samples, j);
    
    // 生成s子集
    cache.sSubsets = m_combGen->generate(samples, s);
    
    // 为每个j组合生成对应的s子集
    cache.jGroupSSubsets.resize(cache.jCombinations.size());
    for (size_t i = 0; i < cache.jCombinations.size(); ++i) {
        cache.jGroupSSubsets[i] = m_combGen->generate(cache.jCombinations[i], s);
    }
    
    return cache;
}

} // namespace core_algo