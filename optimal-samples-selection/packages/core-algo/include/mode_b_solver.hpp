#pragma once

#include "base_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include "types.hpp"
#include <memory>
#include <vector>
#include <chrono>

namespace core_algo {

class ModeBSolver : public BaseSolver {
protected:
    std::vector<std::vector<int>> jGroups_;
    std::vector<std::vector<int>> candidates_;
    std::vector<std::vector<int>> selectedGroups_;

public:
    explicit ModeBSolver(const Config& config) : BaseSolver(config) {}
    virtual ~ModeBSolver() = default;

    // 执行选择
    virtual std::vector<std::vector<int>> performSelection(
        const std::vector<std::vector<int>>& groups,
        const std::vector<std::vector<int>>& jCombinations,
        const std::vector<std::vector<int>>& sSubsets,
        int j,
        int s
    ) const = 0;

    // 求解问题
    virtual DetailedSolution solve(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j
    ) override = 0;
};

// 工厂函数声明
std::shared_ptr<ModeBSolver> createModeBSolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    std::shared_ptr<CoverageCalculator> covCalc,
    const Config& config
);

} // namespace core_algo 