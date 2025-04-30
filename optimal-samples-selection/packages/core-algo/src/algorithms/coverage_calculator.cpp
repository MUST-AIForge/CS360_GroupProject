#include "coverage_calculator.hpp"
#include "utils/timer.hpp"
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <numeric>

namespace core_algo {

namespace {

    class CoverageCalculatorImpl : public CoverageCalculator {
public:
    explicit CoverageCalculatorImpl(const Config& config = Config()) : config(config) {}

    CoverageResult calculateCoverage(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        CoverageMode mode,
        int min_coverage_count
    ) const override {
        if (k_groups.empty() || j_combinations.empty()) {
            return {0.0, 0, static_cast<int>(j_combinations.size()),
                    std::vector<bool>(j_combinations.size(), false),
                    std::vector<int>(j_combinations.size(), 0)};
        }

        Timer timer("计算覆盖率");
        
        // 预处理：将k元组中的所有元素放入哈希集合
        std::unordered_set<int> k_elements;
        for (const auto& group : k_groups) {
            k_elements.insert(group.begin(), group.end());
        }

        CoverageResult result;
        result.total_j_count = j_combinations.size();
        result.j_coverage_status.resize(j_combinations.size(), false);
        result.j_covered_s_counts.resize(j_combinations.size(), 0);
        result.covered_j_count = 0;

        // 对于Mode C，我们需要特殊处理
        if (mode == CoverageMode::CoverAllS) {
            // 为每个k组创建哈希集
            std::vector<std::unordered_set<int>> k_group_sets;
            k_group_sets.reserve(k_groups.size());
            for (const auto& group : k_groups) {
                k_group_sets.emplace_back(group.begin(), group.end());
            }

            for (size_t idx = 0; idx < j_combinations.size(); ++idx) {
                const auto& j = j_combinations[idx];
                if (j.empty()) continue;

                // 检查是否存在一个k组完全覆盖当前j组合
                bool found_covering_group = false;
                for (const auto& k_set : k_group_sets) {
                    bool all_covered = true;
                    for (int element : j) {
                        if (k_set.count(element) == 0) {
                            all_covered = false;
                            break;
                }
                    }
                    if (all_covered) {
                        found_covering_group = true;
                        result.j_coverage_status[idx] = true;
                        result.j_covered_s_counts[idx] = j.size();
                        result.covered_j_count++;
                        break;
                }
                }
                
                // 如果没有找到完全覆盖的k组，记录被覆盖的元素数量
                if (!found_covering_group) {
                    int covered_count = 0;
                    for (int element : j) {
                        for (const auto& k_set : k_group_sets) {
                            if (k_set.count(element) > 0) {
                                covered_count++;
                                break;
                }
                        }
                    }
                    result.j_covered_s_counts[idx] = covered_count;
            }
        }
        } else {
            // 对于其他模式，使用并行处理
            std::mutex result_mutex;
            auto process_range = [&](int start, int end) {
                int local_count = 0;
                std::vector<bool> local_coverage_status(end - start, false);
                std::vector<int> local_covered_counts(end - start, 0);

                for (int idx = start; idx < end; ++idx) {
                    const auto& j = j_combinations[idx];
                    bool is_j_covered = false;
                    int covered_s_count = 0;

                    for (int s : j) {
                        if (k_elements.count(s) > 0) {
                            covered_s_count++;
                        }
                    }

                    switch (mode) {
                        case CoverageMode::CoverMinOneS:
                            is_j_covered = (covered_s_count > 0);
                            break;
                        case CoverageMode::CoverMinNS:
                            is_j_covered = (covered_s_count >= min_coverage_count);
                            break;
                        default:
                            throw AlgorithmError("不支持的覆盖模式");
                    }

                    local_coverage_status[idx - start] = is_j_covered;
                    local_covered_counts[idx - start] = covered_s_count;
                    if (is_j_covered) {
                        local_count++;
                    }
            }

                std::lock_guard<std::mutex> lock(result_mutex);
                for (int i = 0; i < end - start; ++i) {
                    result.j_coverage_status[start + i] = local_coverage_status[i];
                    result.j_covered_s_counts[start + i] = local_covered_counts[i];
        }
                result.covered_j_count += local_count;
            };

            const int num_threads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads;
            const int chunk_size = (j_combinations.size() + num_threads - 1) / num_threads;

            for (int i = 0; i < num_threads && i * chunk_size < j_combinations.size(); ++i) {
                int start = i * chunk_size;
                int end = std::min(start + chunk_size, static_cast<int>(j_combinations.size()));
                threads.emplace_back(process_range, start, end);
            }

            for (auto& thread : threads) {
                thread.join();
            }
        }

        result.coverage_ratio = static_cast<double>(result.covered_j_count) / result.total_j_count;
        return result;
    }

private:
    Config config;
};

} // anonymous namespace

// 工厂方法实现
std::unique_ptr<CoverageCalculator> CoverageCalculator::create(const Config& config) {
    return std::make_unique<CoverageCalculatorImpl>(config);
}

} // namespace core_algo 