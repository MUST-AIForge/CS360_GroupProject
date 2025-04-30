#pragma once

#include <vector>
#include <memory>
#include "types.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"

namespace core_algo {

class ModeCSetCoverSolver {
public:
    virtual ~ModeCSetCoverSolver() = default;

    // 解决集合覆盖问题
    // @param universeSize: 全集大小
    // @param n: 样本数量
    // @param samples: 输入样本
    // @param k: 每个组的大小
    // @param s: 每个元素至少出现的次数
    // @param j: j参数
    // @return DetailedSolution 包含结果和详细信息
    virtual DetailedSolution solve(int universeSize, int n, const std::vector<int>& samples, int k, int s, int j) = 0;

    // 验证解的正确性
    virtual bool verifySolution(
        const std::vector<int>& samples,   // 初始样本集合
        int s,                             // 目标子集大小
        const Solution& solution           // 待验证的解
    ) const = 0;

    // 计算解的质量指标
    virtual std::vector<double> calculateMetrics(
        const std::vector<int>& samples,   // 初始样本集合
        int s,                             // 目标子集大小
        const Solution& solution           // 待评估的解
    ) const = 0;

    // 创建求解器实例
    static std::shared_ptr<ModeCSetCoverSolver> createModeCSetCoverSolver(
        std::shared_ptr<CombinationGenerator> combGen,
        std::shared_ptr<SetOperations> setOps,
        const Config& config = Config()
    );

protected:
    // 构建覆盖矩阵
    virtual std::vector<std::vector<bool>> buildCoverageMatrix(
        const std::vector<std::vector<int>>& universe,    // 所有s-元子集
        const std::vector<std::vector<int>>& candidates   // 所有k-元子集
    ) = 0;

    // 贪心选择函数
    virtual size_t selectNextSet(
        const std::vector<std::vector<bool>>& coverageMatrix,
        const std::vector<bool>& isCovered,
        const std::vector<bool>& isSelected
    ) = 0;
};

} // namespace core_algo 