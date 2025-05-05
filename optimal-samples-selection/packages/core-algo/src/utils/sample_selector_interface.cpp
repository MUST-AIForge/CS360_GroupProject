#include "sample_selector_interface.hpp"
#include "mode_a_solver.hpp"
#include "mode_b_solver.hpp"
#include "mode_c_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <memory>
#include <stdexcept>
#include <unordered_set>

namespace core_algo {

DetailedSolution SampleSelectorInterface::run(
    int m,
    int n,
    int k,
    int s,
    int j,
    int N,
    const std::vector<int>& samples
) {
    // 参数有效性检查
    if (m < 45 || m > 54) {
        throw std::invalid_argument("总样本空间大小m必须在45到54之间");
    }
    if (n < 7 || n > 25 || n > m) {
        throw std::invalid_argument("选择的样本数量n必须在7到25之间，且不能大于m");
    }
    if (k < 4 || k > 7) {
        throw std::invalid_argument("每个组的大小k必须在4到7之间");
    }
    if (j > k) {
        throw std::invalid_argument("j-group的大小j不能大于k");
    }
    if (s < 3 || s > 7 || s > j) {
        throw std::invalid_argument("子集大小s必须在3到7之间，且不能大于j");
    }

    // 计算每个j组合中s子集的总数
    int total_s_subsets = 1;
    for (int i = 0; i < s; i++) {
        total_s_subsets *= (j - i);
        total_s_subsets /= (i + 1);
    }

    // 验证N值的有效性
    if (N < 1 || N > total_s_subsets) {
        throw std::invalid_argument("N值必须在1到s子集总数之间");
    }

    // j和s的特殊关系检查
    bool valid_j_s_relation = false;
    switch (j) {
        case 4:
            valid_j_s_relation = (s == 3 || s == 4);
            break;
        case 5:
            valid_j_s_relation = (s >= 3 && s <= 5);
            break;
        case 6:
            valid_j_s_relation = (s >= 3 && s <= 6);
            break;
        case 7:
            valid_j_s_relation = (s >= 3 && s <= 7);
            break;
        default:
            throw std::invalid_argument("j的值必须在4到7之间");
    }
    if (!valid_j_s_relation) {
        throw std::invalid_argument("s的值与j的关系不符合要求");
    }

    // 检查samples的有效性
    if (samples.empty()) {
        throw std::invalid_argument("samples不能为空");
    }
    if (samples.size() != n) {
        throw std::invalid_argument("samples的大小必须等于n");
    }
    // 检查samples中的值是否在1到m的范围内，且不重复
    std::unordered_set<int> unique_samples(samples.begin(), samples.end());
    if (unique_samples.size() != samples.size()) {
        throw std::invalid_argument("samples中的值不能重复");
    }
    for (int sample : samples) {
        if (sample < 1 || sample > m) {
            throw std::invalid_argument("samples中的值必须在1到m的范围内");
        }
    }

    Config config;
    config.minCoverageCount = N;  // 设置最小覆盖数量
    auto combGen = std::shared_ptr<CombinationGenerator>(CombinationGenerator::create());
    auto setOps = std::shared_ptr<SetOperations>(SetOperations::create(config));
    auto covCalc = CoverageCalculator::create(config);
    DetailedSolution solution;
    
    // 根据N值选择合适的求解器
    if (N == 1) {
        // 使用Mode A求解器
            auto solver = createModeASetCoverSolver(combGen, setOps, std::move(covCalc), config);
        if (!solver) throw std::runtime_error("无法创建求解器");
        solution = solver->solve(m, n, samples, k, s, j);
    } else if (N == total_s_subsets) {
        // 使用Mode C求解器
        auto solver = ModeCSetCoverSolver::createModeCSetCoverSolver(combGen, setOps, config);
        if (!solver) throw std::runtime_error("无法创建求解器");
            solution = solver->solve(m, n, samples, k, s, j);
    } else {
        // 使用Mode B求解器
            auto solver = createModeBSetCoverSolver(combGen, setOps, config);
        if (!solver) throw std::runtime_error("无法创建求解器");
            solution = solver->solve(m, n, samples, k, s, j, N);
    }

    return solution;
}

} // namespace core_algo 