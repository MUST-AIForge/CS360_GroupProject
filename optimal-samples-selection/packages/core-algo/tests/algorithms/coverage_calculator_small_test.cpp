#include <gtest/gtest.h>
#include "coverage_calculator.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "types.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <random>

using namespace core_algo;

class CoverageCalculatorSmallTest : public ::testing::Test {
protected:
    Config config;
    std::unique_ptr<CoverageCalculator> coverageCalc;
    std::unique_ptr<CombinationGenerator> combGen;

    void SetUp() override {
        config.enableRandomization = false;  // 禁用随机化以确保结果一致
        coverageCalc = CoverageCalculator::create(config);
        combGen = CombinationGenerator::create(config);
    }

    void TearDown() override {
        coverageCalc.reset();
        combGen.reset();
    }

    // 辅助函数：打印测试结果
    void printTestResult(const std::string& testName,
                        const std::vector<std::vector<int>>& k_groups,
                        const std::vector<std::vector<int>>& j_combinations,
                        const std::vector<std::vector<std::vector<int>>>& s_subsets,
                        const CoverageResult& result,
                        double expectedCoverage) {
        std::cout << "\n=== " << testName << " ===" << std::endl;
        
        std::cout << "K组合:" << std::endl;
        for (const auto& group : k_groups) {
            std::cout << "  {";
            for (int elem : group) std::cout << elem << " ";
            std::cout << "}" << std::endl;
        }

        std::cout << "\nJ组合及其覆盖状态:" << std::endl;
        for (size_t i = 0; i < j_combinations.size(); ++i) {
            std::cout << "  {";
            for (int elem : j_combinations[i]) std::cout << elem << " ";
            std::cout << "} - " << (result.j_coverage_status[i] ? "已覆盖" : "未覆盖");
            std::cout << " (覆盖的S子集数: " << result.j_covered_s_counts[i] << ")" << std::endl;
            
            std::cout << "  S子集:" << std::endl;
            for (const auto& subset : s_subsets[i]) {
                std::cout << "    {";
                for (int elem : subset) std::cout << elem << " ";
                std::cout << "}" << std::endl;
            }
        }

        std::cout << "\n实际覆盖率: " << result.coverage_ratio;
        std::cout << "\n覆盖的j组数量: " << result.covered_j_count << "/" << result.total_j_count;
        std::cout << "\n总组数: " << result.total_groups << std::endl;
        std::cout << "\n期望覆盖率: " << expectedCoverage << std::endl;
    }
};

// 测试用例1：简单的完全覆盖情况
TEST_F(CoverageCalculatorSmallTest, SimpleFullCoverage) {
    // k_groups: {{1,2,3}}
    // j_combinations: {{1,2,3}, {2,3,4}}  // 确保每个j组合都是3个元素
    std::vector<std::vector<int>> k_groups = {{1,2,3}};
    std::vector<std::vector<int>> j_combinations = {{1,2,3}, {2,3,4}};
    
    // 手动生成每个j组合的s子集（大小为2）
    std::vector<std::vector<std::vector<int>>> s_subsets = {
        {{1,2}, {1,3}, {2,3}},  // j1的s子集
        {{2,3}, {2,4}, {3,4}}   // j2的s子集
    };

    // Mode A: 只需要一个s子集被覆盖
    auto resultA = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinOneS, 1);
    printTestResult("Mode A - Simple Full Coverage", k_groups, j_combinations, s_subsets, 
                   resultA, 1.0);  // 两个j组合都有s子集被覆盖
    EXPECT_DOUBLE_EQ(resultA.coverage_ratio, 1.0);
    EXPECT_EQ(resultA.covered_j_count, 2);

    // Mode B: 需要至少2个s子集被覆盖
    auto resultB = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinNS, 2);
    printTestResult("Mode B - Simple Full Coverage", k_groups, j_combinations, s_subsets, 
                   resultB, 0.5);  // 只有第一个j组合有足够的s子集被覆盖
    EXPECT_DOUBLE_EQ(resultB.coverage_ratio, 0.5);
    EXPECT_EQ(resultB.covered_j_count, 1);

    // Mode C: 所有s子集都需要被覆盖
    auto resultC = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverAllS, 1);
    printTestResult("Mode C - Simple Full Coverage", k_groups, j_combinations, s_subsets, 
                   resultC, 0.5);  // 只有第一个j组合的所有s子集都被覆盖
    EXPECT_DOUBLE_EQ(resultC.coverage_ratio, 0.5);
    EXPECT_EQ(resultC.covered_j_count, 1);
}

// 测试用例2：部分覆盖情况
TEST_F(CoverageCalculatorSmallTest, PartialCoverage) {
    // k_groups: {{1,2}, {3,4}}
    // j_combinations: {{1,2,3}, {2,3,4}, {3,4,5}}  // 确保每个j组合都是3个元素
    std::vector<std::vector<int>> k_groups = {{1,2}, {3,4}};
    std::vector<std::vector<int>> j_combinations = {{1,2,3}, {2,3,4}, {3,4,5}};
    
    // 手动生成每个j组合的s子集（大小为2）
    std::vector<std::vector<std::vector<int>>> s_subsets = {
        {{1,2}, {1,3}, {2,3}},  // j1的s子集
        {{2,3}, {2,4}, {3,4}},  // j2的s子集
        {{3,4}, {3,5}, {4,5}}   // j3的s子集
    };

    // Mode A: 只需要一个s子集被覆盖
    auto resultA = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinOneS, 1);
    printTestResult("Mode A - Partial Coverage", k_groups, j_combinations, s_subsets, 
                   resultA, 1.0);  // 所有j组合都有至少一个s子集被覆盖
    EXPECT_NEAR(resultA.coverage_ratio, 1.0, 0.001);
    EXPECT_EQ(resultA.covered_j_count, 3);

    // Mode B: 需要至少2个s子集被覆盖
    auto resultB = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinNS, 2);
    printTestResult("Mode B - Partial Coverage", k_groups, j_combinations, s_subsets, 
                   resultB, 0.0);  // 没有j组合有足够的s子集被覆盖
    EXPECT_NEAR(resultB.coverage_ratio, 0.0, 0.001);
    EXPECT_EQ(resultB.covered_j_count, 0);

    // Mode C: 所有s子集都需要被覆盖
    auto resultC = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverAllS, 1);
    printTestResult("Mode C - Partial Coverage", k_groups, j_combinations, s_subsets, 
                   resultC, 0.0);  // 没有j组合的所有s子集都被覆盖
    EXPECT_DOUBLE_EQ(resultC.coverage_ratio, 0.0);
    EXPECT_EQ(resultC.covered_j_count, 0);
}

// 测试用例3：边界情况
TEST_F(CoverageCalculatorSmallTest, EdgeCases) {
    // 空集测试
    std::vector<std::vector<int>> empty_k_groups;
    std::vector<std::vector<int>> empty_j_combinations;
    std::vector<std::vector<std::vector<int>>> empty_s_subsets;

    auto resultEmpty = coverageCalc->calculateCoverage(empty_k_groups, empty_j_combinations, 
                                                     empty_s_subsets, CoverageMode::CoverMinOneS, 1);
    EXPECT_DOUBLE_EQ(resultEmpty.coverage_ratio, 0.0);
    EXPECT_EQ(resultEmpty.covered_j_count, 0);

    // 单元素测试（每个j组合仍然保持相同大小）
    std::vector<std::vector<int>> single_k_groups = {{1}};
    std::vector<std::vector<int>> single_j_combinations = {{1,2,3}};
    std::vector<std::vector<std::vector<int>>> single_s_subsets = {
        {{1,2}, {1,3}, {2,3}}  // 手动生成s子集
    };

    auto resultSingle = coverageCalc->calculateCoverage(single_k_groups, single_j_combinations, 
                                                      single_s_subsets, CoverageMode::CoverMinOneS, 1);
    EXPECT_DOUBLE_EQ(resultSingle.coverage_ratio, 0.0);
    EXPECT_EQ(resultSingle.covered_j_count, 0);
}

// 测试用例4：较大规模的覆盖测试
TEST_F(CoverageCalculatorSmallTest, LargerScaleCoverage) {
    // k_groups: 5个组合，每个包含3-4个元素
    std::vector<std::vector<int>> k_groups = {
        {1, 2, 3, 4},
        {2, 3, 4, 5},
        {3, 4, 5, 6},
        {4, 5, 6, 7},
        {1, 3, 5, 7}
    };

    // j_combinations: 8个组合，每个包含3个元素
    std::vector<std::vector<int>> j_combinations = {
        {1, 2, 3},
        {2, 3, 4},
        {3, 4, 5},
        {4, 5, 6},
        {5, 6, 7},
        {1, 3, 5},
        {2, 4, 6},
        {3, 5, 7}
    };
    
    // 为每个j组合生成所有可能的大小为2的s子集
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_comb : j_combinations) {
        std::vector<std::vector<int>> j_subsets;
        // 生成所有可能的2元素组合
        for (size_t i = 0; i < j_comb.size(); ++i) {
            for (size_t j = i + 1; j < j_comb.size(); ++j) {
                j_subsets.push_back({j_comb[i], j_comb[j]});
            }
        }
        s_subsets.push_back(j_subsets);
    }

    // Mode A: 只需要一个s子集被覆盖
    auto resultA = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinOneS, 1);
    printTestResult("Mode A - Larger Scale Coverage", k_groups, j_combinations, s_subsets, 
                   resultA, 1.0);  // 实际计算得到100%
    EXPECT_DOUBLE_EQ(resultA.coverage_ratio, 1.0);  // 所有j组合都有至少一个s子集被覆盖
    EXPECT_EQ(resultA.covered_j_count, 8);          // 所有8个j组合都被覆盖

    // Mode B: 需要至少2个s子集被覆盖
    auto resultB = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinNS, 2);
    printTestResult("Mode B - Larger Scale Coverage", k_groups, j_combinations, s_subsets, 
                   resultB, 1.0);  // 实际计算得到100%
    EXPECT_DOUBLE_EQ(resultB.coverage_ratio, 1.0);  // 所有j组合都有至少2个s子集被覆盖
    EXPECT_EQ(resultB.covered_j_count, 8);          // 所有8个j组合都被覆盖

    // Mode C: 所有s子集都需要被覆盖
    auto resultC = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverAllS, 1);
    printTestResult("Mode C - Larger Scale Coverage", k_groups, j_combinations, s_subsets, 
                   resultC, 0.875);  // 实际计算得到87.5%
    EXPECT_DOUBLE_EQ(resultC.coverage_ratio, 0.875);  // 7/8的j组合的所有s子集都被覆盖
    EXPECT_EQ(resultC.covered_j_count, 7);            // 7个j组合被完全覆盖
}

// 测试用例5：重叠元素的覆盖测试
TEST_F(CoverageCalculatorSmallTest, OverlappingCoverage) {
    // k_groups: 4个组合，有重叠的元素
    std::vector<std::vector<int>> k_groups = {
        {1, 2, 3, 4},
        {3, 4, 5, 6},
        {5, 6, 7, 8},
        {7, 8, 1, 2}
    };

    // j_combinations: 6个组合，每个包含4个元素，有重叠
    std::vector<std::vector<int>> j_combinations = {
        {1, 2, 3, 4},
        {2, 3, 4, 5},
        {3, 4, 5, 6},
        {4, 5, 6, 7},
        {5, 6, 7, 8},
        {6, 7, 8, 1}
    };
    
    // 为每个j组合生成所有可能的大小为3的s子集
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_comb : j_combinations) {
        std::vector<std::vector<int>> j_subsets;
        // 生成所有可能的3元素组合
        for (size_t i = 0; i < j_comb.size() - 2; ++i) {
            for (size_t j = i + 1; j < j_comb.size() - 1; ++j) {
                for (size_t k = j + 1; k < j_comb.size(); ++k) {
                    j_subsets.push_back({j_comb[i], j_comb[j], j_comb[k]});
                }
            }
        }
        s_subsets.push_back(j_subsets);
    }

    // Mode A: 只需要一个s子集被覆盖
    auto resultA = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinOneS, 1);
    printTestResult("Mode A - Overlapping Coverage", k_groups, j_combinations, s_subsets, 
                   resultA, 1.0);  // 精确计算得到100%
    EXPECT_DOUBLE_EQ(resultA.coverage_ratio, 1.0);  // 期望精确的100%覆盖率
    EXPECT_EQ(resultA.covered_j_count, 6);          // 期望精确的6个j组合被覆盖

    // Mode B: 需要至少2个s子集被覆盖
    auto resultB = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinNS, 2);
    printTestResult("Mode B - Overlapping Coverage", k_groups, j_combinations, s_subsets, 
                   resultB, 1.0);  // 精确计算得到100%
    EXPECT_DOUBLE_EQ(resultB.coverage_ratio, 1.0);  // 期望精确的100%覆盖率
    EXPECT_EQ(resultB.covered_j_count, 6);          // 期望精确的6个j组合被覆盖

    // Mode C: 所有s子集都需要被覆盖
    auto resultC = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverAllS, 1);
    printTestResult("Mode C - Overlapping Coverage", k_groups, j_combinations, s_subsets, 
                   resultC, 0.5);  // 精确计算得到50%
    EXPECT_DOUBLE_EQ(resultC.coverage_ratio, 0.5);  // 期望精确的50%覆盖率
    EXPECT_EQ(resultC.covered_j_count, 3);          // 期望精确的3个j组合被覆盖
}

// 测试用例6：超大规模性能测试 (k=7, j=6, s=5)
TEST_F(CoverageCalculatorSmallTest, LargeScalePerformanceTest) {
    const int NUM_K_GROUPS = 1000;   // k组数量增加到1000个
    const int NUM_J_COMBS = 10000;   // j组数量增加到10000个
    const int K_SIZE = 7;            // 每个k组的大小
    const int J_SIZE = 6;            // 每个j组的大小
    const int S_SIZE = 5;            // 每个s子集的大小
    const int MAX_ELEM = 30;         // 最大元素值增加到30

    std::cout << "\n=== 超大规模性能测试 ===" << std::endl;
    std::cout << "配置参数:" << std::endl;
    std::cout << "- k组数量: " << NUM_K_GROUPS << std::endl;
    std::cout << "- j组数量: " << NUM_J_COMBS << std::endl;
    std::cout << "- k组大小: " << K_SIZE << std::endl;
    std::cout << "- j组大小: " << J_SIZE << std::endl;
    std::cout << "- s子集大小: " << S_SIZE << std::endl;
    std::cout << "- 最大元素值: " << MAX_ELEM << std::endl;

    auto start_total = std::chrono::high_resolution_clock::now();

    // 生成k组
    std::cout << "\n1. 生成k组..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> k_groups;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, MAX_ELEM);

    for (int i = 0; i < NUM_K_GROUPS; ++i) {
        std::vector<int> group;
        while (group.size() < K_SIZE) {
            int val = dis(gen);
            if (std::find(group.begin(), group.end(), val) == group.end()) {
                group.push_back(val);
            }
        }
        std::sort(group.begin(), group.end());
        k_groups.push_back(group);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "k组生成完成，耗时: " << duration << "ms" << std::endl;

    // 生成j组合
    std::cout << "\n2. 生成j组合..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<int>> j_combinations;
    for (int i = 0; i < NUM_J_COMBS; ++i) {
        std::vector<int> comb;
        while (comb.size() < J_SIZE) {
            int val = dis(gen);
            if (std::find(comb.begin(), comb.end(), val) == comb.end()) {
                comb.push_back(val);
            }
        }
        std::sort(comb.begin(), comb.end());
        j_combinations.push_back(comb);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "j组合生成完成，耗时: " << duration << "ms" << std::endl;

    // 生成s子集
    std::cout << "\n3. 生成s子集..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_comb : j_combinations) {
        std::vector<std::vector<int>> j_subsets;
        // 生成所有可能的5元素组合
        for (size_t i = 0; i < j_comb.size() - 4; ++i) {
            for (size_t j = i + 1; j < j_comb.size() - 3; ++j) {
                for (size_t k = j + 1; k < j_comb.size() - 2; ++k) {
                    for (size_t l = k + 1; l < j_comb.size() - 1; ++l) {
                        for (size_t m = l + 1; m < j_comb.size(); ++m) {
                            j_subsets.push_back({j_comb[i], j_comb[j], j_comb[k], j_comb[l], j_comb[m]});
                        }
                    }
                }
            }
        }
        s_subsets.push_back(j_subsets);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "s子集生成完成，耗时: " << duration << "ms" << std::endl;
    std::cout << "每个j组的s子集数量: " << s_subsets[0].size() << std::endl;

    // 执行覆盖率计算
    std::cout << "\n4. 执行覆盖率计算..." << std::endl;

    // Mode A
    start = std::chrono::high_resolution_clock::now();
    auto resultA = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinOneS, 1);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Mode A 执行时间: " << duration << "ms" << std::endl;
    std::cout << "Mode A 覆盖的j组数量: " << resultA.covered_j_count << "/" << j_combinations.size() << std::endl;

    // Mode B
    start = std::chrono::high_resolution_clock::now();
    auto resultB = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverMinNS, 3);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Mode B 执行时间: " << duration << "ms" << std::endl;
    std::cout << "Mode B 覆盖的j组数量: " << resultB.covered_j_count << "/" << j_combinations.size() << std::endl;

    // Mode C
    start = std::chrono::high_resolution_clock::now();
    auto resultC = coverageCalc->calculateCoverage(k_groups, j_combinations, s_subsets, 
                                                 CoverageMode::CoverAllS, 1);
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Mode C 执行时间: " << duration << "ms" << std::endl;
    std::cout << "Mode C 覆盖的j组数量: " << resultC.covered_j_count << "/" << j_combinations.size() << std::endl;

    auto end_total = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count();
    std::cout << "\n总执行时间: " << total_duration << "ms" << std::endl;

    // 只验证数据结构的正确性，不验证覆盖率
    for (const auto& group : k_groups) {
        EXPECT_EQ(group.size(), K_SIZE) << "k组大小应该为" << K_SIZE;
    }
    for (const auto& j_comb : j_combinations) {
        EXPECT_EQ(j_comb.size(), J_SIZE) << "j组大小应该为" << J_SIZE;
    }
    for (const auto& s_subset_group : s_subsets) {
        for (const auto& subset : s_subset_group) {
            EXPECT_EQ(subset.size(), S_SIZE) << "s子集大小应该为" << S_SIZE;
        }
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 