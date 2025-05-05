#include "coverage_calculator.hpp"
#include "timer.hpp"
#include "set_operations.hpp"
#include <algorithm>
#include <unordered_set>
#include <thread>
#include <vector>
#include <mutex>
#include <numeric>
#include <iostream>
#include <atomic>
#include <chrono>

namespace core_algo {

namespace {

// 覆盖计算策略基类
class CoverageStrategy {
public:
    CoverageStrategy() : set_ops(SetOperations::create()) {}
    virtual ~CoverageStrategy() = default;

    virtual CoverageResult calculate(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        const std::vector<std::vector<std::vector<int>>>& s_subsets,
        int min_coverage_count
    ) const = 0;

protected:
    std::unique_ptr<SetOperations> set_ops;

    // 辅助函数：初始化结果结构
    CoverageResult initializeResult(size_t j_size) const {
        return CoverageResult(
            0.0,                                    // coverage_ratio
            0,                                      // covered_j_count
            static_cast<int>(j_size),              // total_j_count
            std::vector<bool>(j_size, false),      // j_coverage_status
            std::vector<int>(j_size, 0),           // j_covered_s_counts
            0                                      // total_groups
        );
    }

    // 辅助函数：计算覆盖率
    void finalizeResult(CoverageResult& result) const {
        if (result.total_j_count == 0) {
            result.coverage_ratio = 0.0;
        } else {
            result.coverage_ratio = static_cast<double>(result.covered_j_count) / result.total_j_count;
        }
    }

    // 辅助函数：检查一个s子集是否被任意k组完全覆盖
    bool isSSubsetCoveredByAnyKGroup(
        const std::vector<int>& s_subset,
        const std::vector<std::vector<int>>& k_groups
    ) const {
        if (s_subset.empty()) return false;
        
        for (const auto& k_group : k_groups) {
            bool covered = true;
            for (int elem : s_subset) {
                if (std::find(k_group.begin(), k_group.end(), elem) == k_group.end()) {
                    covered = false;
                    break;
                }
            }
            if (covered) return true;
        }
        
        return false;
    }
};

// Mode A: 对每个j，检查是否至少有一个大小为s的子集被k组完全覆盖
class CoverMinOneStrategy : public CoverageStrategy {
public:
    CoverageResult calculate(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        const std::vector<std::vector<std::vector<int>>>& s_subsets,
        int min_coverage_count
    ) const override {
        Timer timer("Mode A 覆盖计算");
        
        std::cout << "\n=== Mode A 覆盖计算开始 ===" << std::endl;
        std::cout << "输入参数分析:" << std::endl;
        std::cout << "- k组数量: " << k_groups.size() << std::endl;
        std::cout << "- j组数量: " << j_combinations.size() << std::endl;
        std::cout << "- 每个j组的s子集数量: " << (s_subsets.empty() ? 0 : s_subsets[0].size()) << std::endl;
        std::cout << "- 最小覆盖要求: " << min_coverage_count << std::endl;
        
        // 打印k组信息
        std::cout << "\nk组列表:" << std::endl;
        for (const auto& group : k_groups) {
            std::cout << "  组: ";
            for (int elem : group) {
                std::cout << elem << " ";
            }
            std::cout << std::endl;
        }
        
        auto result = initializeResult(j_combinations.size());
        result.total_groups = static_cast<int>(k_groups.size());
        
        // 优化线程数量：根据数据规模和CPU核心数动态调整
        const size_t data_size = j_combinations.size();
        const unsigned int available_threads = std::thread::hardware_concurrency();
        const unsigned int optimal_threads = std::min(
            available_threads,
            static_cast<unsigned int>((data_size + 99) / 100)  // 每100个项目分配一个线程
        );
        const size_t chunk_size = (data_size + optimal_threads - 1) / optimal_threads;
        
        std::cout << "\n线程配置:" << std::endl;
        std::cout << "- 可用CPU线程数: " << available_threads << std::endl;
        std::cout << "- 优化后使用线程数: " << optimal_threads << std::endl;
        std::cout << "- 每线程处理数据量: " << chunk_size << std::endl;
        
        std::vector<std::thread> threads;
        std::atomic<int> covered_count{0};
        std::mutex result_mutex;
        std::mutex cout_mutex;  // 用于同步输出

        auto process_range = [&](size_t start, size_t end) {
            std::vector<bool> local_coverage_status(end - start, false);
            std::vector<int> local_covered_s_counts(end - start, 0);
            int local_covered_count = 0;

            // 处理每个j组合
            for (size_t i = start; i < end; ++i) {
                bool j_group_covered = false;
                int covered_subsets = 0;
                
                // 检查当前j组合的所有s子集
                for (const auto& s_subset : s_subsets[i]) {
                    if (isSSubsetCoveredByAnyKGroup(s_subset, k_groups)) {
                        covered_subsets++;
                        j_group_covered = true;
                        break;  // 只要有一个s子集被覆盖就可以了
                    }
                }
                
                local_covered_s_counts[i - start] = covered_subsets;
                if (j_group_covered) {
                    local_coverage_status[i - start] = true;
                    local_covered_count++;
                }
            }

            // 更新全局结果
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                for (size_t i = 0; i < end - start; ++i) {
                    result.j_coverage_status[start + i] = local_coverage_status[i];
                    result.j_covered_s_counts[start + i] = local_covered_s_counts[i];
                }
                covered_count += local_covered_count;
            }
        };

        // 创建并启动线程
        threads.reserve(optimal_threads);
        for (unsigned int i = 0; i < optimal_threads && i * chunk_size < j_combinations.size(); ++i) {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, j_combinations.size());
            threads.emplace_back(process_range, start, end);
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        result.covered_j_count = covered_count;
        finalizeResult(result);
        
        std::cout << "\n=== 覆盖计算结果 ===" << std::endl;
        std::cout << "- 总j组数量: " << result.total_j_count << std::endl;
        std::cout << "- 已覆盖j组数量: " << result.covered_j_count << std::endl;
        std::cout << "- 覆盖率: " << (result.coverage_ratio * 100) << "%" << std::endl;
        std::cout << "=== 覆盖计算完成 ===" << std::endl;
        
        return result;
    }
};

// Mode B: 对每个j，检查是否有至少N个不同的大小为s的子集被k组完全覆盖
class CoverMinNStrategy : public CoverageStrategy {
public:
    CoverageResult calculate(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        const std::vector<std::vector<std::vector<int>>>& s_subsets,
        int min_coverage_count
    ) const override {
        Timer timer("Mode B 覆盖计算");

        auto result = initializeResult(j_combinations.size());
        result.total_groups = static_cast<int>(k_groups.size());

        // 优化线程数量
        const size_t data_size = j_combinations.size();
        const unsigned int available_threads = std::thread::hardware_concurrency();
        const unsigned int optimal_threads = std::min(
            available_threads,
            static_cast<unsigned int>((data_size + 99) / 100)
        );
        const size_t chunk_size = (data_size + optimal_threads - 1) / optimal_threads;

        std::vector<std::thread> threads;
        std::atomic<int> covered_count{0};
        std::mutex result_mutex;

        auto process_range = [&](size_t start, size_t end) {
            std::vector<bool> local_coverage_status(end - start, false);
            std::vector<int> local_covered_s_counts(end - start, 0);
            int local_covered_count = 0;

            for (size_t i = start; i < end; ++i) {
                int covered_subsets = 0;
                
                // 检查每个s子集是否被任何k组覆盖
                for (const auto& s_subset : s_subsets[i]) {
                    if (isSSubsetCoveredByAnyKGroup(s_subset, k_groups)) {
                        covered_subsets++;
                    }
                }
                
                local_covered_s_counts[i - start] = covered_subsets;
                
                // 如果达到所需的最小覆盖数量，标记为已覆盖
                if (covered_subsets >= min_coverage_count) {
                    local_coverage_status[i - start] = true;
                    local_covered_count++;
                }
            }

            // 更新全局结果
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                for (size_t i = 0; i < end - start; ++i) {
                    result.j_coverage_status[start + i] = local_coverage_status[i];
                    result.j_covered_s_counts[start + i] = local_covered_s_counts[i];
                }
                covered_count += local_covered_count;
            }
        };

        // 创建并启动线程
        threads.reserve(optimal_threads);
        for (unsigned int i = 0; i < optimal_threads && i * chunk_size < j_combinations.size(); ++i) {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, j_combinations.size());
            threads.emplace_back(process_range, start, end);
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        result.covered_j_count = covered_count;
        finalizeResult(result);
        return result;
    }
};

// Mode C: 对每个j，检查所有大小为s的子集是否都被k组完全覆盖
class CoverAllStrategy : public CoverageStrategy {
public:
    CoverageResult calculate(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        const std::vector<std::vector<std::vector<int>>>& s_subsets,
        int min_coverage_count
    ) const override {
        Timer timer("Mode C 覆盖计算");

        auto result = initializeResult(j_combinations.size());
        result.total_groups = static_cast<int>(k_groups.size());

        // 使用标准线程库进行并行处理
        std::mutex result_mutex;
        const unsigned int available_threads = std::thread::hardware_concurrency();
        const unsigned int optimal_threads = std::min(
            available_threads,
            static_cast<unsigned int>((j_combinations.size() + 99) / 100)
        );
        const size_t chunk_size = (j_combinations.size() + optimal_threads - 1) / optimal_threads;
        std::vector<std::thread> threads;
        std::atomic<int> covered_count{0};

        auto process_range = [&](size_t start, size_t end) {
            std::vector<bool> local_coverage_status(end - start, false);
            std::vector<int> local_covered_s_counts(end - start, 0);
            int local_covered_count = 0;

            for (size_t i = start; i < end; ++i) {
                bool all_subsets_covered = true;
                int covered_subsets = 0;

                // 检查每个s子集是否被任何k组覆盖
                for (const auto& s_subset : s_subsets[i]) {
                    if (isSSubsetCoveredByAnyKGroup(s_subset, k_groups)) {
                        covered_subsets++;
                    } else {
                        all_subsets_covered = false;
                        break;  // 如果有一个子集未被覆盖，就不需要继续检查
                    }
                }

                local_covered_s_counts[i - start] = covered_subsets;
                if (all_subsets_covered && covered_subsets > 0) {  // 确保至少有一个子集且全部被覆盖
                    local_coverage_status[i - start] = true;
                    local_covered_count++;
                }
            }

            // 更新全局结果
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                for (size_t i = 0; i < end - start; ++i) {
                    result.j_coverage_status[start + i] = local_coverage_status[i];
                    result.j_covered_s_counts[start + i] = local_covered_s_counts[i];
                }
                covered_count += local_covered_count;
            }
        };

        // 创建并启动线程
        threads.reserve(optimal_threads);
        for (unsigned int i = 0; i < optimal_threads && i * chunk_size < j_combinations.size(); ++i) {
            size_t start = i * chunk_size;
            size_t end = std::min(start + chunk_size, j_combinations.size());
            threads.emplace_back(process_range, start, end);
        }

        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }

        result.covered_j_count = covered_count;
        finalizeResult(result);
        return result;
    }
};

} // namespace

class CoverageCalculatorImpl : public CoverageCalculator {
private:
    std::unique_ptr<CoverageStrategy> createStrategy(CoverageMode mode) const {
        switch (mode) {
            case CoverageMode::CoverMinOneS:
                return std::make_unique<CoverMinOneStrategy>();
            case CoverageMode::CoverMinNS:
                return std::make_unique<CoverMinNStrategy>();
            case CoverageMode::CoverAllS:
                return std::make_unique<CoverAllStrategy>();
            default:
                throw std::invalid_argument("未知的覆盖模式");
        }
    }

public:
    CoverageResult calculateCoverage(
        const std::vector<std::vector<int>>& k_groups,
        const std::vector<std::vector<int>>& j_combinations,
        const std::vector<std::vector<std::vector<int>>>& s_subsets,
        CoverageMode mode,
        int min_coverage_count
    ) const override {
        auto strategy = createStrategy(mode);
        return strategy->calculate(k_groups, j_combinations, s_subsets, min_coverage_count);
    }
};

std::unique_ptr<CoverageCalculator> CoverageCalculator::create(const Config& config) {
    return std::make_unique<CoverageCalculatorImpl>();
}

} // namespace core_algo 