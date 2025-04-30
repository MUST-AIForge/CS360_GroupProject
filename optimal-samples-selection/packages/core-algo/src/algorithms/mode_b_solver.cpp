#include "mode_b_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <chrono>
#include <algorithm>
#include <numeric>
#include <unordered_set>
#include <random>
#include <stdexcept>
#include <cmath>
#include <queue>
#include <map>
#include <iostream>
#include <future>
#include <thread>
#include <set>

namespace core_algo {

namespace {
    // 为vector<int>定义比较器
    struct VectorCompare {
        bool operator()(const std::vector<int>& a, const std::vector<int>& b) const {
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
        }
    };

    // 并行生成组合的辅助函数
    std::vector<std::vector<int>> generateCombinationsParallel(
        std::shared_ptr<CombinationGenerator> combGen,
        const std::vector<int>& elements,
        int size
    ) {
        const int numThreads = std::thread::hardware_concurrency();
        std::vector<std::future<std::vector<std::vector<int>>>> futures;
        
        auto combinations = combGen->generate(elements, size);
        if (combinations.size() < numThreads) {
            return combinations;
        }

        const size_t chunkSize = combinations.size() / numThreads;
        std::vector<std::vector<std::vector<int>>> results(numThreads);

        for (int i = 0; i < numThreads; ++i) {
            size_t start = i * chunkSize;
            size_t end = (i == numThreads - 1) ? combinations.size() : (i + 1) * chunkSize;
            futures.push_back(std::async(std::launch::async, [start, end, &combinations]() {
                std::vector<std::vector<int>> chunk;
                chunk.insert(chunk.end(), combinations.begin() + start, combinations.begin() + end);
                return chunk;
            }));
        }

        std::vector<std::vector<int>> result;
        for (auto& future : futures) {
            auto chunk = future.get();
            result.insert(result.end(), chunk.begin(), chunk.end());
        }
        return result;
    }

    class ModeBSetCoverSolverImpl : public ModeBSetCoverSolver {
    public:
        ModeBSetCoverSolverImpl(
            std::shared_ptr<CombinationGenerator> combGen,
            std::shared_ptr<SetOperations> setOps,
            const Config& config
        )
            : combinationGenerator_(std::move(combGen))
            , setOperations_(std::move(setOps))
            , config_(config)
            , s_(0)
            , candidates_()
            , jGroups_()
            , selectedGroups_()
        {}

        DetailedSolution solve(int m, int n, const std::vector<int>& samples, int k, int s, int j, int N) override {
            DetailedSolution solution;
            s_ = s;  // 保存s值供后续使用
            
            // 输入验证
            if (samples.empty() || k <= 0 || s <= 0 || k < s || N <= 0 || j <= 0 || j > n) {
                solution.status = Status::NoSolution;
                solution.message = "Invalid input parameters";
                return solution;
            }

            try {
                auto startTime = std::chrono::high_resolution_clock::now();

                // 生成所有j大小的组合（jGroups）
                jGroups_ = generateCombinationsParallel(combinationGenerator_, samples, j);
                // 生成所有k大小的候选组
                candidates_ = generateCombinationsParallel(combinationGenerator_, samples, k);
                selectedGroups_.clear();

                // 为每个j-group生成所有可能的s子集
                std::vector<std::vector<std::vector<int>>> jGroupSubsets(jGroups_.size());
                for (size_t i = 0; i < jGroups_.size(); ++i) {
                    jGroupSubsets[i] = combinationGenerator_->generate(jGroups_[i], s);
                }

                // 贪心选择过程
                while (true) {
                    // 检查每个j-group中已覆盖的不同s子集数量
                    std::vector<std::set<std::vector<int>, VectorCompare>> coveredSubsets(jGroups_.size());
                    for (const auto& group : selectedGroups_) {
                        for (size_t i = 0; i < jGroups_.size(); ++i) {
                            for (const auto& subset : jGroupSubsets[i]) {
                                if (setOperations_->contains(group, subset)) {
                                    coveredSubsets[i].insert(subset);
                                }
                            }
                        }
                    }

                    bool allCovered = true;
                    for (size_t i = 0; i < jGroups_.size(); ++i) {
                        if (coveredSubsets[i].size() < static_cast<size_t>(N)) {
                            allCovered = false;
                            break;
                        }
                    }
                    if (allCovered) break;

                    // 评估剩余的候选组
                    std::vector<bool> candidateUsed(candidates_.size(), false);
                    for (const auto& selected : selectedGroups_) {
                        for (size_t i = 0; i < candidates_.size(); ++i) {
                            if (candidates_[i] == selected) {
                                candidateUsed[i] = true;
                                break;
                            }
                        }
                    }

                    // 选择下一个最佳集合
                    double bestScore = -1;
                    size_t bestCandidate = candidates_.size();

                    for (size_t i = 0; i < candidates_.size(); ++i) {
                        if (candidateUsed[i]) continue;

                        double score = 0;
                        for (size_t j = 0; j < jGroups_.size(); ++j) {
                            if (coveredSubsets[j].size() >= static_cast<size_t>(N)) continue;

                            std::set<std::vector<int>, VectorCompare> newCoveredSubsets;
                            for (const auto& subset : jGroupSubsets[j]) {
                                if (setOperations_->contains(candidates_[i], subset)) {
                                    if (coveredSubsets[j].find(subset) == coveredSubsets[j].end()) {
                                        newCoveredSubsets.insert(subset);
                                    }
                                }
                            }
                            
                            // 计算这个候选组能为当前j-group新增多少个未覆盖的s子集
                            size_t remainingNeeded = N - coveredSubsets[j].size();
                            score += std::min(newCoveredSubsets.size(), remainingNeeded);
                        }

                        if (score > bestScore) {
                            bestScore = score;
                            bestCandidate = i;
                        }
                    }

                    if (bestCandidate == candidates_.size() || bestScore <= 0) break;

                    // 添加最佳候选组
                    selectedGroups_.push_back(candidates_[bestCandidate]);
                }

                // 检查最终结果
                std::vector<size_t> coveredSubsetCount(jGroups_.size(), 0);
                for (size_t i = 0; i < jGroups_.size(); ++i) {
                    std::set<std::vector<int>, VectorCompare> uniqueSubsets;
                    for (const auto& group : selectedGroups_) {
                        for (const auto& subset : jGroupSubsets[i]) {
                            if (setOperations_->contains(group, subset)) {
                                uniqueSubsets.insert(subset);
                            }
                        }
                    }
                    coveredSubsetCount[i] = uniqueSubsets.size();
                }

                bool allCovered = true;
                for (size_t i = 0; i < jGroups_.size(); ++i) {
                    if (coveredSubsetCount[i] < static_cast<size_t>(N)) {
                        allCovered = false;
                        break;
                    }
                }

                if (allCovered) {
                    solution.status = Status::Success;
                    solution.message = "Solution found successfully";
                    solution.groups = selectedGroups_;
                    solution.totalGroups = static_cast<int>(selectedGroups_.size());
                    solution.isOptimal = false;  // 贪心算法不保证最优解
                    solution.coverageRatio = 1.0;  // 所有jGroups都有N个不同的s子集被覆盖
                } else {
                    solution.status = Status::NoSolution;
                    solution.message = "Could not cover N different s-subsets for all jGroups";
                    // 计算覆盖率
                    size_t totalRequired = jGroups_.size() * N;
                    size_t totalCovered = std::accumulate(coveredSubsetCount.begin(), coveredSubsetCount.end(), 0);
                    solution.coverageRatio = static_cast<double>(totalCovered) / totalRequired;
                }

                auto endTime = std::chrono::high_resolution_clock::now();
                solution.computationTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000000.0;
                return solution;

            } catch (const std::exception& e) {
                solution.status = Status::Error;
                solution.message = std::string("Error during computation: ") + e.what();
                return solution;
            }
        }

        std::vector<double> calculateMetrics(
            const std::vector<int>& samples,
            int s,
            const Solution& solution
        ) const override {
            std::vector<double> metrics;
            if (solution.groups.empty()) {
                return {0.0, 0.0};  // 返回默认指标
            }

            // 计算平均覆盖率
            auto jGroups = combinationGenerator_->generate(samples, s);
            std::vector<int> coverageCount(jGroups.size(), 0);
            
            for (const auto& group : solution.groups) {
                for (size_t i = 0; i < jGroups.size(); ++i) {
                    auto subsets = combinationGenerator_->generate(jGroups[i], s);
                    for (const auto& subset : subsets) {
                        if (setOperations_->contains(group, subset)) {
                            coverageCount[i]++;
                            break;  // 每个组只计算一次覆盖
                        }
                    }
                }
            }

            double avgCoverage = 0.0;
            for (int count : coverageCount) {
                avgCoverage += count;
            }
            avgCoverage /= jGroups.size();

            // 计算组大小的标准差
            double avgSize = 0.0;
            for (const auto& group : solution.groups) {
                avgSize += group.size();
            }
            avgSize /= solution.groups.size();

            return {avgCoverage, avgSize};
        }

    protected:
        std::vector<std::vector<bool>> buildCoverageMatrix(
            const std::vector<std::vector<int>>& universe,
            const std::vector<std::vector<int>>& candidates
        ) override {
            std::vector<std::vector<bool>> matrix(candidates.size(), std::vector<bool>(jGroups_.size(), false));
            for (size_t i = 0; i < candidates.size(); ++i) {
                for (size_t j = 0; j < jGroups_.size(); ++j) {
                    matrix[i][j] = setOperations_->contains(candidates[i], jGroups_[j]);
                }
            }
            return matrix;
        }

        bool verifySolution(
            const std::vector<int>& samples,
            int s,
            int N,
            const Solution& solution
        ) const override {
            if (solution.groups.empty()) {
                return false;
            }

            // 生成所有j大小的组合
            auto jGroups = combinationGenerator_->generate(samples, s_);
            
            // 对每个j-group，检查有多少个不同的s子集被覆盖
            for (const auto& jGroup : jGroups) {
                // 生成该j-group的所有s子集
                auto subsets = combinationGenerator_->generate(jGroup, s);
                
                // 记录被覆盖的不同s子集
                std::set<std::vector<int>, VectorCompare> coveredSubsets;
                
                // 检查每个选中的组是否覆盖了新的s子集
                for (const auto& group : solution.groups) {
                    for (const auto& subset : subsets) {
                        if (setOperations_->contains(group, subset)) {
                            coveredSubsets.insert(subset);
                        }
                    }
                }
                
                // 如果该j-group没有N个不同的s子集被覆盖，返回false
                if (coveredSubsets.size() < static_cast<size_t>(N)) {
                    return false;
                }
            }
            
            return true;
        }

        virtual size_t selectNextSet(
            const std::vector<std::vector<bool>>& coverageMatrix,
            const std::vector<int>& jGroupCoveredCount,
            const std::vector<bool>& candidateUsed,
            int N
        ) override {
            double bestScore = -1;
            size_t bestCandidate = candidates_.size();  // 默认返回无效索引

            for (size_t i = 0; i < candidates_.size(); ++i) {
                if (candidateUsed[i]) continue;

                double score = 0;
                for (size_t j = 0; j < jGroups_.size(); ++j) {
                    if (coverageMatrix[i][j]) {
                        // 如果j-group还没有达到所需的覆盖次数，增加分数
                        if (jGroupCoveredCount[j] < N) {
                            // 离目标覆盖次数越远，分数越高
                            score += (N - jGroupCoveredCount[j]);
                        }
                    }
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestCandidate = i;
                }
            }

            return bestCandidate;
        }

    private:
        std::shared_ptr<CombinationGenerator> combinationGenerator_;
        std::shared_ptr<SetOperations> setOperations_;
        Config config_;
        int s_;  // 当前的s值
        std::vector<std::vector<int>> candidates_;  // 所有候选组
        std::vector<std::vector<int>> jGroups_;     // 所有j-groups
        std::vector<std::vector<int>> selectedGroups_;  // 已选择的组
    };

} // anonymous namespace

// 工厂函数实现
std::shared_ptr<ModeBSetCoverSolver> createModeBSetCoverSolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    const Config& config
) {
    if (!combGen || !setOps) {
        throw std::invalid_argument("Invalid arguments: combGen and setOps must not be null");
    }
    return std::make_shared<ModeBSetCoverSolverImpl>(combGen, setOps, config);
}

} // namespace core_algo

