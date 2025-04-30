#pragma once

#include <vector>
#include <memory>
#include "types.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"

namespace core_algo {

class ModeBSetCoverSolver {
public:
    virtual ~ModeBSetCoverSolver() = default;

    // 主要求解函数
    // m: 总样本数量
    // n: 需要选择的样本数量
    // samples: 输入的样本集合
    // k: 每组的大小
    // s: 子集大小
    // j: 额外的参数
    // N: 最小覆盖数量
    virtual DetailedSolution solve(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j,
        int N
    ) = 0;

protected:
    // 验证解的正确性
    virtual bool verifySolution(
        const std::vector<int>& samples,
        int s,
        int N,
        const Solution& solution
    ) const = 0;

    // 计算性能指标
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
        const std::vector<int>& coverCount,
        const std::vector<bool>& isSelected,
        int N
    ) = 0;
};

// 工厂函数
std::shared_ptr<ModeBSetCoverSolver> createModeBSetCoverSolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    const Config& config = Config()
);

} // namespace core_algo 