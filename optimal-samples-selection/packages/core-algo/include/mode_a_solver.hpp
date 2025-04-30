#pragma once

#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include "base_solver.hpp"
#include "types.hpp"
#include <memory>
#include <vector>

namespace core_algo {

class ModeASetCoverSolver : public BaseSetCoverSolver {
public:
    ModeASetCoverSolver(const Config& config) : BaseSetCoverSolver(config) {}
    virtual ~ModeASetCoverSolver() = default;

    // 主要求解函数
    virtual DetailedSolution solve(
        int m,              // 总样本数量
        int n,              // 从m中选择的样本数量
        const std::vector<int>& samples,  // 输入样本
        int k,              // 每组样本数量
        int s,              // 需要覆盖的子集大小
        int j              // j参数
    ) = 0;

protected:
    // 验证解的正确性
    virtual bool verifySolution(
        const std::vector<int>& samples,
        int s,
        int coverCount,
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
    ) const = 0;

    // 选择下一个最优集合
    virtual size_t selectNextSet(
        const std::vector<std::vector<bool>>& coverageMatrix,
        const std::vector<bool>& isCovered,
        const std::vector<bool>& isSelected,
        int coverCount
    ) = 0;
};

// 工厂函数
std::shared_ptr<ModeASetCoverSolver> createModeASetCoverSolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    std::shared_ptr<CoverageCalculator> covCalc,
    const Config& config
);

} // namespace core_algo 