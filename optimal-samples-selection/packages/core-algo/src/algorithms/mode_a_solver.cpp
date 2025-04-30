#include "mode_a_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace core_algo {

namespace {
    class ModeASetCoverSolverImpl : public ModeASetCoverSolver {
    private:
        std::shared_ptr<CombinationGenerator> m_combGen;
        std::shared_ptr<SetOperations> m_setOps;
        Config m_config;

    public:
        ModeASetCoverSolverImpl(
            std::shared_ptr<CombinationGenerator> combGen,
            std::shared_ptr<SetOperations> setOps,
            const Config& config)
            : ModeASetCoverSolver(config), m_combGen(combGen), m_setOps(setOps), m_config(config) {}

        DetailedSolution solve(int universeSize, int n, const std::vector<int>& samples,
                             int k, int s, int j) override {
            auto start = std::chrono::high_resolution_clock::now();
            DetailedSolution solution;
                
                // 参数验证
            if (!BaseSetCoverSolver::validateParameters(universeSize, n, k, s, j)) {
                    solution.status = Status::NoSolution;
                solution.message = "Invalid input parameters";
                solution.coverageRatio = 0.0;
                solution.totalGroups = 0;
                solution.computationTime = 0.0;
                solution.isOptimal = false;
                solution.metrics = std::vector<double>{0.0, 0.0, 0.0};
            return solution;
        }

            try {
                // 生成所有s大小的子集
                auto sSubsets = m_combGen->generate(samples, s);
        
                // 生成所有k大小的候选组
                auto kGroups = m_combGen->generate(samples, k);
        
                // 找到能覆盖所有s子集的最小k组集合
                std::vector<std::vector<int>> selectedGroups;
                std::vector<bool> covered(sSubsets.size(), false);
                std::vector<bool> isSelected(kGroups.size(), false);
            
                // 贪心算法：每次选择能覆盖最多未覆盖子集的k组
                while (selectedGroups.size() < kGroups.size()) {
                    size_t bestIndex = selectNextSet(buildCoverageMatrix(sSubsets, kGroups), covered, isSelected, j);
                    if (bestIndex >= kGroups.size()) break;
                    
                    selectedGroups.push_back(kGroups[bestIndex]);
                    isSelected[bestIndex] = true;
                    
                    // 更新覆盖状态
                    for (size_t i = 0; i < sSubsets.size(); i++) {
                        if (!covered[i] && m_setOps->contains(kGroups[bestIndex], sSubsets[i])) {
                            covered[i] = true;
                        }
                    }
                    
                    // 检查是否所有子集都被覆盖
                    bool allCovered = true;
                    for (bool c : covered) {
                        if (!c) {
                            allCovered = false;
                            break;
                        }
                    }
                    if (allCovered) break;
        }

                // 检查是否找到解
                bool allCovered = true;
                for (bool c : covered) {
                    if (!c) {
                        allCovered = false;
                        break;
                }
                }
                
                if (!allCovered) {
                    solution.status = Status::NoSolution;
                    solution.message = "Could not find a solution that covers all subsets";
                    solution.coverageRatio = 0.0;
                    solution.totalGroups = 0;
                    solution.computationTime = 0.0;
                    solution.isOptimal = false;
                    solution.metrics = std::vector<double>{0.0, 0.0, 0.0};
                    return solution;
        }

                // 计算指标
                solution.status = Status::Success;
                solution.groups = selectedGroups;
                solution.coverageRatio = 1.0;
                solution.totalGroups = selectedGroups.size();
                
                // 计算组间多样性
                double totalDiversity = 0.0;
                int pairCount = 0;
                for (size_t i = 0; i < selectedGroups.size(); i++) {
                    for (size_t j = i + 1; j < selectedGroups.size(); j++) {
                        double similarity = m_setOps->calculateJaccardSimilarity(
                            selectedGroups[i], selectedGroups[j]);
                        totalDiversity += (1.0 - similarity);
                        pairCount++;
                    }
                }
                
                std::vector<double> metrics;
                metrics.push_back(1.0); // 覆盖率
                metrics.push_back(pairCount > 0 ? totalDiversity / pairCount : 0.0); // 多样性
                metrics.push_back(1.0 / selectedGroups.size()); // 效率
                solution.metrics = metrics;
                
                auto end = std::chrono::high_resolution_clock::now();
                solution.computationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - start).count() / 1000.0;
                solution.isOptimal = false;
                
                return solution;
            } catch (const std::exception& e) {
                solution.status = Status::Error;
                solution.message = std::string("Error during solving: ") + e.what();
                return solution;
        }
        }

        bool verifySolution(const std::vector<int>& samples, int s,
                           const Solution& solution) const override {
            if (solution.groups.empty()) {
                return false;
                }
                
            // 生成所有s大小的子集
            auto subsets = m_combGen->generate(samples, s);
            
            // 检查每个s大小的子集是否被至少一个组覆盖
            for (const auto& subset : subsets) {
                bool covered = false;
                for (const auto& group : solution.groups) {
                    if (m_setOps->contains(group, subset)) {
                        covered = true;
                    break;
                    }
                }
                if (!covered) {
                    return false;
                }
            }
            
            return true;
            }
            
        std::vector<std::vector<bool>> buildCoverageMatrix(
            const std::vector<std::vector<int>>& universe,
            const std::vector<std::vector<int>>& candidates
        ) override {
            std::vector<std::vector<bool>> coverageMatrix(universe.size(), 
                std::vector<bool>(candidates.size(), false));
            
            for (size_t i = 0; i < universe.size(); i++) {
                for (size_t j = 0; j < candidates.size(); j++) {
                    coverageMatrix[i][j] = m_setOps->contains(candidates[j], universe[i]);
            }
            }
            
            return coverageMatrix;
        }

        size_t selectNextSet(
            const std::vector<std::vector<bool>>& coverageMatrix,
            const std::vector<bool>& isCovered,
            const std::vector<bool>& isSelected
        ) override {
            size_t bestIndex = coverageMatrix[0].size();  // 默认无效索引
            int maxNewCovered = 0;
            
            for (size_t j = 0; j < coverageMatrix[0].size(); j++) {
                if (isSelected[j]) continue;
                
                    int newCovered = 0;
                for (size_t i = 0; i < coverageMatrix.size(); i++) {
                    if (!isCovered[i] && coverageMatrix[i][j]) {
                            newCovered++;
                        }
                    }
                    
                if (newCovered > maxNewCovered) {
                    maxNewCovered = newCovered;
                    bestIndex = j;
                }
            }
            
            return bestIndex;
        }

        bool verifySolution(const std::vector<int>& samples, int s, int coverCount,
                           const Solution& solution) const override {
            if (solution.groups.empty()) {
                return false;
            }

            // 生成所有s大小的子集
            auto subsets = m_combGen->generate(samples, s);
            
            // 检查每个s大小的子集是否被至少coverCount个组覆盖
            for (const auto& subset : subsets) {
                int coverageCount = 0;
                for (const auto& group : solution.groups) {
                    if (m_setOps->contains(group, subset)) {
                        coverageCount++;
                        if (coverageCount >= coverCount) {
                        break;
                    }
                }
            }
                if (coverageCount < coverCount) {
                    return false;
                }
            }
            
            return true;
        }

        std::vector<double> calculateMetrics(const std::vector<int>& samples, int s,
                                           const Solution& solution) const override {
            std::vector<double> metrics(3, 0.0);
            if (solution.groups.empty()) {
                return metrics;
            }
            
            // 计算覆盖率
            auto subsets = m_combGen->generate(samples, s);
            int coveredCount = 0;
            for (const auto& subset : subsets) {
                for (const auto& group : solution.groups) {
                    if (m_setOps->contains(group, subset)) {
                        coveredCount++;
                        break;
                    }
                }
            }
            metrics[0] = static_cast<double>(coveredCount) / subsets.size();

            // 计算组间多样性
            double totalDiversity = 0.0;
            int pairCount = 0;
            for (size_t i = 0; i < solution.groups.size(); i++) {
                for (size_t j = i + 1; j < solution.groups.size(); j++) {
                    double similarity = m_setOps->calculateJaccardSimilarity(
                        solution.groups[i], solution.groups[j]);
                    totalDiversity += (1.0 - similarity);
                    pairCount++;
                }
            }
            metrics[1] = pairCount > 0 ? totalDiversity / pairCount : 0.0;
            
            // 计算效率（组数的倒数）
            metrics[2] = 1.0 / solution.groups.size();

            return metrics;
        }

        std::vector<std::vector<bool>> buildCoverageMatrix(
            const std::vector<std::vector<int>>& universe,
            const std::vector<std::vector<int>>& candidates
        ) const override {
            std::vector<std::vector<bool>> coverageMatrix(universe.size(), 
                std::vector<bool>(candidates.size(), false));
            
            for (size_t i = 0; i < universe.size(); i++) {
                for (size_t j = 0; j < candidates.size(); j++) {
                    coverageMatrix[i][j] = m_setOps->contains(candidates[j], universe[i]);
                }
            }
            
            return coverageMatrix;
        }

        size_t selectNextSet(
            const std::vector<std::vector<bool>>& coverageMatrix,
            const std::vector<bool>& isCovered,
            const std::vector<bool>& isSelected,
            int coverCount
        ) override {
            size_t bestIndex = coverageMatrix[0].size();  // 默认无效索引
            int maxNewCovered = 0;
            
            for (size_t j = 0; j < coverageMatrix[0].size(); j++) {
                if (isSelected[j]) continue;
                
                int newCovered = 0;
                for (size_t i = 0; i < coverageMatrix.size(); i++) {
                    if (!isCovered[i] && coverageMatrix[i][j]) {
                        newCovered++;
                    }
                }
                
                if (newCovered > maxNewCovered) {
                    maxNewCovered = newCovered;
                    bestIndex = j;
                }
            }
            
            return bestIndex;
        }
    };
} // anonymous namespace

// 工厂函数实现
std::shared_ptr<ModeASetCoverSolver> createModeASetCoverSolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    std::shared_ptr<CoverageCalculator> covCalc,
    const Config& config
) {
    if (!combGen || !setOps) {
        throw std::invalid_argument("Invalid arguments: combGen and setOps must not be null");
    }
    return std::shared_ptr<ModeASetCoverSolver>(new ModeASetCoverSolverImpl(combGen, setOps, config));
}

} // namespace core_algo