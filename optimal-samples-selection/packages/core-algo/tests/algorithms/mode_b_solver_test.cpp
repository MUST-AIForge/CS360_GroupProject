#include <gtest/gtest.h>
#include "mode_b_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <map>
#include <set>

namespace core_algo {

class ModeBSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        Config config;
        m_combGen = CombinationGenerator::create(config);
        m_setOps = SetOperations::create(config);
        m_solver = createModeBSetCoverSolver(m_combGen, m_setOps, config);
    }

    bool verifyCoverage(const std::vector<std::vector<int>>& groups,
                       const std::vector<int>& samples,
                       int s,
                       int j,
                       int N) {
        auto jGroups = m_combGen->generate(samples, j);
        
        // 对每个j-group检查被覆盖的不同s子集数量
        for (const auto& jGroup : jGroups) {
            auto sSubsets = m_combGen->generate(jGroup, s);
            std::set<std::vector<int>> coveredSubsets;
            
            // 对于每个选中的组
            for (const auto& group : groups) {
                // 检查这个组覆盖了哪些s子集
                for (const auto& subset : sSubsets) {
                    if (m_setOps->contains(group, subset)) {
                        coveredSubsets.insert(subset);
                    }
                }
            }
            
            // 检查这个j-group是否有足够多的不同s子集被覆盖
            if (coveredSubsets.size() < static_cast<size_t>(N)) {
                return false;
            }
        }
        
        return true;
    }

    DetailedSolution runWithTimeout(int m, int n, const std::vector<int>& samples,
                                  int k, int s, int j, int N, int timeoutSeconds) {
        auto future = std::async(std::launch::async, [&]() {
            return m_solver->solve(m, n, samples, k, s, j, N);
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

    // 检查每个jGroup的s子集至少有N个被覆盖
    void checkJGroupCoverageN(const DetailedSolution& solution, const std::vector<int>& samples, int j, int s, int N) {
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
            int coveredCount = 0;
            for (size_t idx : groupSubsets[i]) {
                if (isCovered[idx]) coveredCount++;
            }
            EXPECT_GE(coveredCount, N) << "jGroup被覆盖的s子集数不足N";
        }
    }

    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<ModeBSetCoverSolver> m_solver;
};

TEST_F(ModeBSolverTest, EmptyInput) {
    std::vector<int> samples;
    auto result = m_solver->solve(0, 0, samples, 0, 0, 0, 0);
    EXPECT_EQ(result.status, Status::NoSolution);
}

TEST_F(ModeBSolverTest, InvalidParameters) {
    std::vector<int> samples = {1, 2, 3};
    auto result = m_solver->solve(3, 2, samples, 4, 2, 4, 2);
    EXPECT_EQ(result.status, Status::NoSolution);
}

TEST_F(ModeBSolverTest, BasicCoverage) {
    std::vector<int> samples = {1, 2, 3, 4, 5};
    int N = 2;  // 每个j-group至少被覆盖2次
    auto result = m_solver->solve(5, 5, samples, 3, 2, 3, N);
    
    EXPECT_EQ(result.status, Status::Success);
    EXPECT_TRUE(verifyCoverage(result.groups, samples, 2, 3, N))
        << "每个j-group应该至少被覆盖" << N << "次";
}

TEST_F(ModeBSolverTest, LargeInput) {
    std::vector<int> samples = {1, 2, 3, 4, 5, 6};
    int N = 2;  // 每个j-group至少被覆盖2次
    auto result = m_solver->solve(6, 6, samples, 3, 2, 3, N);
    
    EXPECT_EQ(result.status, Status::Success);
    EXPECT_TRUE(verifyCoverage(result.groups, samples, 2, 3, N))
        << "每个j-group应该至少被覆盖" << N << "次";
}

TEST_F(ModeBSolverTest, EdgeCases) {
    // 测试k=s的情况
    std::vector<int> samples = {1, 2, 3, 4};
    int N = 1;  // 每个j-group至少被覆盖1次
    auto result = m_solver->solve(4, 4, samples, 2, 2, 2, N);
    
    EXPECT_EQ(result.status, Status::Success);
    EXPECT_TRUE(verifyCoverage(result.groups, samples, 2, 2, N))
        << "每个j-group应该至少被覆盖" << N << "次";
}

}  // namespace core_algo 