#include "mode_a_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <numeric>
#include <map>

namespace core_algo {
namespace testing {

class ModeASetCoverSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config config;
        m_combGen = CombinationGenerator::create(config);
        m_setOps = SetOperations::create(config);
        m_covCalc = CoverageCalculator::create(config);
        m_solver = createModeASetCoverSolver(m_combGen, m_setOps, m_covCalc, config);
    }

    void TearDown() override {}

    // 检查每个jGroup是否至少有一个s子集被覆盖
    bool checkJGroupCoverage(const DetailedSolution& solution, const std::vector<int>& samples, int j, int s) {
        auto jGroups = m_combGen->generate(samples, j);
        auto allSubsets = m_combGen->generate(samples, s);
        
        // 为每个j组找到其所有s子集
        std::vector<std::vector<std::vector<int>>> jGroupSubsets;
        for (const auto& jGroup : jGroups) {
            jGroupSubsets.push_back(m_combGen->generate(jGroup, s));
        }

        // 检查每个j组是否至少有一个s子集被覆盖
        for (size_t i = 0; i < jGroups.size(); ++i) {
            bool thisJGroupCovered = false;
            for (const auto& subset : jGroupSubsets[i]) {
                for (const auto& group : solution.groups) {
                    if (m_setOps->contains(group, subset)) {
                        thisJGroupCovered = true;
                        break;
                    }
                }
                if (thisJGroupCovered) break;
            }
            if (!thisJGroupCovered) return false;
        }
        return true;
    }

    std::shared_ptr<ModeASetCoverSolver> m_solver;
    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<CoverageCalculator> m_covCalc;
};

// 基本功能测试：验证是否能正确覆盖所有j组
TEST_F(ModeASetCoverSolverTest, BasicCoverage) {
    std::vector<int> samples = {1,2,3,4,5};
    int m = 5, n = 5, k = 3, s = 2, j = 3;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s));
}

// 边缘情况测试：验证在最小参数下是否能找到解
TEST_F(ModeASetCoverSolverTest, MinimalSolution) {
    std::vector<int> samples = {1,2,3,4};
    int m = 4, n = 4, k = 2, s = 2, j = 2;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s));
}

// 参数验证测试：验证是否正确处理无效参数
TEST_F(ModeASetCoverSolverTest, InvalidParameters) {
    std::vector<int> samples = {1,2,3,4,5};
    int m = 5, n = 5, k = 3, s = 2, j = 3;
    
    // k > n 的情况
    auto solution = m_solver->solve(m, n, samples, 6, s, j);
    EXPECT_EQ(solution.status, Status::NoSolution);
    
    // s > k 的情况
    solution = m_solver->solve(m, n, samples, k, 4, j);
    EXPECT_EQ(solution.status, Status::NoSolution);
    
    // j > n 的情况
    solution = m_solver->solve(m, n, samples, k, s, 6);
    EXPECT_EQ(solution.status, Status::NoSolution);
}

} // namespace testing
} // namespace core_algo 