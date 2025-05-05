#include <gtest/gtest.h>
#include "mode_c_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <future>
#include <numeric>
#include <cmath>
#include <map>

// 添加 GTest main 函数
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

namespace core_algo {
namespace test {

class ModeCSetCoverSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config config;
        config.enableParallel = true;
        m_combGen = CombinationGenerator::create(config);
        m_setOps = SetOperations::create(config);
        m_covCalc = CoverageCalculator::create(config);
        m_solver = createModeCSetCoverSolver(m_combGen, m_setOps, m_covCalc, config);
    }

    void TearDown() override {
        m_solver.reset();
        m_combGen.reset();
        m_setOps.reset();
        m_covCalc.reset();
    }

    void recordTestProperties(const DetailedSolution& solution) {
        testing::Test::RecordProperty("Status", static_cast<int>(solution.status));
        testing::Test::RecordProperty("Message", solution.message);
        testing::Test::RecordProperty("ComputationTime", solution.computationTime);
        testing::Test::RecordProperty("CoverageRatio", solution.coverageRatio);
        testing::Test::RecordProperty("TotalCombinations", solution.totalGroups);
    }

    double evaluateSolutionQuality(const DetailedSolution& solution,
                                 const std::vector<int>& samples,
                                 int s) {
        if (solution.groups.empty()) return 0.0;

        auto subsets = m_combGen->generate(samples, s);
        std::vector<bool> covered(subsets.size(), false);
        int coveredCount = 0;
        
        for (size_t i = 0; i < subsets.size(); i++) {
            for (const auto& group : solution.groups) {
                if (!covered[i] && m_setOps->contains(group, subsets[i])) {
                    covered[i] = true;
                    coveredCount++;
                    break;
                }
            }
        }

        return static_cast<double>(coveredCount) / subsets.size();
    }

    double evaluateSolutionEfficiency(const DetailedSolution& solution,
                                    const std::vector<int>& samples,
                                    int s) {
        if (solution.groups.empty()) return 0.0;

        auto subsets = m_combGen->generate(samples, s);
        double totalCoverage = 0.0;
        
        for (const auto& subset : subsets) {
            int coverCount = 0;
            for (const auto& group : solution.groups) {
                if (m_setOps->contains(group, subset)) {
                    coverCount++;
                }
            }
            totalCoverage += coverCount > 0 ? 1.0 / coverCount : 0.0;
        }

        return totalCoverage / subsets.size();
    }

    double evaluateGroupDiversity(const DetailedSolution& solution) {
        if (solution.groups.size() <= 1) return 0.0;

        double totalSimilarity = 0.0;
        int comparisons = 0;

        for (size_t i = 0; i < solution.groups.size(); ++i) {
            for (size_t j = i + 1; j < solution.groups.size(); ++j) {
                totalSimilarity += m_setOps->calculateJaccardSimilarity(
                    solution.groups[i], solution.groups[j]);
                comparisons++;
            }
        }

        return comparisons > 0 ? 1.0 - (totalSimilarity / comparisons) : 0.0;
    }

    DetailedSolution runWithTimeout(int universeSize, int n, const std::vector<int>& samples,
                                  int k, int s, int j, int timeoutSeconds) {
        auto future = std::async(std::launch::async, [&]() {
            return m_solver->solve(universeSize, n, samples, k, s, j);
        });

        auto status = future.wait_for(std::chrono::seconds(timeoutSeconds));
        if (status == std::future_status::timeout) {
            DetailedSolution timeoutSolution;
            timeoutSolution.status = Status::Timeout;
            timeoutSolution.message = "Computation timed out";
            return timeoutSolution;
        }

        return future.get();
    }

    double calculateStdDev(const std::vector<double>& values, double mean) {
        double variance = 0.0;
        for (double value : values) {
            variance += (value - mean) * (value - mean);
        }
        variance /= values.size();
        return std::sqrt(variance);
    }

    // 检查每个jGroup的s子集全部被覆盖
    void checkJGroupFullCoverage(const DetailedSolution& solution, const std::vector<int>& samples, int j, int s) {
        auto jGroups = m_combGen->generate(samples, j);
        auto allSubsets = m_combGen->generate(samples, s);
        std::map<std::vector<int>, size_t> subsetIndex;
        for (size_t i = 0; i < allSubsets.size(); ++i) subsetIndex[allSubsets[i]] = i;
        std::vector<std::vector<size_t>> groupSubsets(jGroups.size());
        for (size_t i = 0; i < jGroups.size(); ++i) {
            auto sSubsets = m_combGen->generate(jGroups[i], s);
            for (const auto& sset : sSubsets) {
                if (subsetIndex.count(sset)) groupSubsets[i].push_back(subsetIndex[sset]);
            }
        }
        std::vector<bool> isCovered(allSubsets.size(), false);
        for (const auto& group : solution.groups) {
            for (size_t i = 0; i < allSubsets.size(); ++i) {
                if (m_setOps->contains(group, allSubsets[i])) isCovered[i] = true;
            }
        }
        for (size_t i = 0; i < jGroups.size(); ++i) {
            for (size_t idx : groupSubsets[i]) {
                EXPECT_TRUE(isCovered[idx]) << "jGroup的某个s子集未被覆盖";
            }
        }
    }

    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<CoverageCalculator> m_covCalc;
    std::shared_ptr<ModeCSetCoverSolver> m_solver;
};

TEST_F(ModeCSetCoverSolverTest, EmptyInput) {
    std::vector<int> samples;
    auto result = m_solver->solve(0, 0, samples, 0, 0, 0);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::NoSolution);
}

TEST_F(ModeCSetCoverSolverTest, BasicFunctionality) {
    std::vector<int> samples = {0, 1, 2, 3, 4, 5};
    auto result = m_solver->solve(6, samples.size(), samples, 4, 3, 4);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::Success);
    EXPECT_FALSE(result.groups.empty());
    
    for (const auto& group : result.groups) {
        EXPECT_EQ(group.size(), 4);
    }
}

TEST_F(ModeCSetCoverSolverTest, CompleteCoverage) {
    std::vector<int> samples = {0, 1, 2, 3, 4};
    auto result = m_solver->solve(5, samples.size(), samples, 3, 2, 3);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::Success);
    
    auto allSubsets = m_combGen->generate(samples, 2);
    
    for (const auto& subset : allSubsets) {
        bool covered = false;
        for (const auto& group : result.groups) {
            if (m_setOps->contains(group, subset)) {
                covered = true;
                break;
            }
        }
        EXPECT_TRUE(covered) << "子集 {" << subset[0] << "," << subset[1] << "} 未被覆盖";
    }
}

TEST_F(ModeCSetCoverSolverTest, MetricsCalculation) {
    std::vector<int> samples = {0, 1, 2, 3, 4, 5};
    auto result = m_solver->solve(6, samples.size(), samples, 4, 3, 4);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::Success);
    EXPECT_FALSE(result.metrics.empty());
    
    if (!result.metrics.empty()) {
        EXPECT_DOUBLE_EQ(result.metrics[0], 4.0);
        
        EXPECT_GE(result.metrics[1], 0.0);
        EXPECT_LE(result.metrics[1], 1.0);
    }
}

TEST_F(ModeCSetCoverSolverTest, DifficultSubsets) {
    std::vector<int> samples = {1, 2, 3, 4, 5};
    auto result = m_solver->solve(5, samples.size(), samples, 3, 2, 3);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::Success);
    
    // 检查每个j组的所有s子集是否都被覆盖
    auto jGroups = m_combGen->generate(samples, 3);
    for (const auto& jGroup : jGroups) {
        auto sSubsets = m_combGen->generate(jGroup, 2);
        for (const auto& subset : sSubsets) {
            bool covered = false;
            for (const auto& group : result.groups) {
                if (m_setOps->contains(group, subset)) {
                    covered = true;
                    break;
                }
            }
            EXPECT_TRUE(covered) << "子集 {" << subset[0] << "," << subset[1] << "} 未被覆盖";
        }
    }
}

TEST_F(ModeCSetCoverSolverTest, BasicJGroupFullCoverage) {
    std::vector<int> samples = {1, 2, 3, 4, 5};
    auto result = m_solver->solve(5, samples.size(), samples, 3, 2, 3);
    recordTestProperties(result);
    EXPECT_EQ(result.status, Status::Success);
    checkJGroupFullCoverage(result, samples, 3, 2);
}

TEST_F(ModeCSetCoverSolverTest, InvalidParameters) {
    std::vector<int> samples = {0, 1, 2, 3, 4};
    
    auto result = m_solver->solve(5, samples.size(), samples, 6, 2, 3);
    EXPECT_EQ(result.status, Status::NoSolution);
    
    result = m_solver->solve(5, samples.size(), samples, 3, 4, 3);
    EXPECT_EQ(result.status, Status::NoSolution);
    
    result = m_solver->solve(5, samples.size(), samples, 3, 2, 6);
    EXPECT_EQ(result.status, Status::NoSolution);
}

TEST_F(ModeCSetCoverSolverTest, EdgeCases) {
    std::vector<int> samples = {0, 1, 2, 3};
    auto result = m_solver->solve(4, samples.size(), samples, 2, 2, 2);
    EXPECT_EQ(result.status, Status::Success);
    
    samples = {0, 1, 2, 3, 4};
    result = m_solver->solve(5, samples.size(), samples, 4, 2, 3);
    EXPECT_EQ(result.status, Status::Success);
}

} // namespace test
} // namespace core_algo 