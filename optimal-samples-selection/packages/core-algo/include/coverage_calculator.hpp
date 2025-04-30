#pragma once

#include "types.hpp"
#include <vector>
#include <memory>

namespace core_algo {

// 覆盖计算结果
struct CoverageResult {
    double coverage_ratio;      // 覆盖率（满足要求的j的比例）
    int covered_j_count;        // 满足要求的j的数量
    int total_j_count;         // 总的j数量
    std::vector<bool> j_coverage_status;  // 每个j是否被覆盖的状态
    std::vector<int> j_covered_s_counts;  // 每个j中被覆盖的s的数量
};

// 覆盖计算器接口
class CoverageCalculator {
public:
    virtual ~CoverageCalculator() = default;

    // 计算k元组对j的覆盖率
    virtual CoverageResult calculateCoverage(
        const std::vector<std::vector<int>>& k_groups,  // 选中的k元组集合
        const std::vector<std::vector<int>>& j_combinations,  // 所有的j组合
        CoverageMode mode,
        int min_coverage_count = 1  // Mode B中使用，表示每个j中最少需要多少个s被覆盖
    ) const = 0;

    // 工厂方法
    static std::unique_ptr<CoverageCalculator> create(
        const Config& config = Config()
    );
};

} // namespace core_algo 