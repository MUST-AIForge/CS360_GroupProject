#pragma once

#include <vector>
#include <memory>
#include "types.hpp"

namespace core_algo {

// 覆盖计算器接口
class CoverageCalculator {
public:
    virtual ~CoverageCalculator() = default;

    // 计算覆盖率
    virtual CoverageResult calculateCoverage(
        const std::vector<std::vector<int>>& k_groups,          // 选中的k元组集合
        const std::vector<std::vector<int>>& j_combinations,    // 所有的j组合
        const std::vector<std::vector<std::vector<int>>>& s_subsets,  // 每个j组合的s子集集合
        CoverageMode mode,                                      // 覆盖模式
        int min_coverage_count = 1                             // Mode B中使用，表示每个j中最少需要多少个s被覆盖
    ) const = 0;

    // 工厂方法
    static std::unique_ptr<CoverageCalculator> create(const Config& config = Config());
};

} // namespace core_algo 