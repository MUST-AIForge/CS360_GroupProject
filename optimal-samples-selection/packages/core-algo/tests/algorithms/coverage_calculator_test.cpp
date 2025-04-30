#include <gtest/gtest.h>
#include "coverage_calculator.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>

using namespace core_algo;

class CoverageCalculatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = Config();
        coverageCalc = CoverageCalculator::create(config);
    }

    void TearDown() override {
        coverageCalc.reset();
    }

    // 生成随机测试数据的辅助函数
    std::vector<std::vector<int>> generateRandomSets(
        size_t numSets, 
        size_t setSize, 
        int minValue = 0, 
        int maxValue = 100
    ) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(minValue, maxValue);

        std::vector<std::vector<int>> sets(numSets);
        for (auto& set : sets) {
            for (size_t i = 0; i < setSize; ++i) {
                set.push_back(dis(gen));
            }
            std::sort(set.begin(), set.end());
            set.erase(std::unique(set.begin(), set.end()), set.end());
        }
        return sets;
    }

    void printCoverageResult(const std::string& mode_name, const CoverageResult& result, 
                           const std::vector<std::vector<int>>& k_groups,
                           const std::vector<std::vector<int>>& j_combinations) {
        std::cout << "\n=== " << mode_name << " 测试结果 ===" << std::endl;
        std::cout << "k_groups (选中的组合):" << std::endl;
        for (size_t i = 0; i < k_groups.size(); ++i) {
            std::cout << "  组 " << i << ": {";
            for (int elem : k_groups[i]) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }

        std::cout << "\nj_combinations (待覆盖的组合):" << std::endl;
        for (size_t i = 0; i < j_combinations.size(); ++i) {
            std::cout << "  组合 " << i << ": {";
            for (int elem : j_combinations[i]) {
                std::cout << elem << " ";
            }
            std::cout << "} - 覆盖状态: " << (result.j_coverage_status[i] ? "是" : "否")
                     << ", 覆盖元素数: " << result.j_covered_s_counts[i] 
                     << ", 组合大小: " << j_combinations[i].size() << std::endl;
        }

        std::cout << "\n覆盖统计:" << std::endl;
        std::cout << "总组合数: " << result.total_j_count << std::endl;
        std::cout << "被覆盖组合数: " << result.covered_j_count << std::endl;
        std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
    }

    Config config;
    std::unique_ptr<CoverageCalculator> coverageCalc;
};

// 基本覆盖率计算测试
TEST_F(CoverageCalculatorTest, BasicCoverageCalculation) {
    std::vector<std::vector<int>> k_groups = {
        {1, 2, 3},
        {3, 4, 5}
    };
    std::vector<std::vector<int>> j_combinations = {
        {1, 2},    // 应该被覆盖 (Mode A, B)，Mode C也应该被覆盖因为{1, 2}在第一个k组中
        {3},       // 应该被覆盖 (所有Mode)
        {4, 5},    // 应该被覆盖 (Mode A, B)，Mode C也应该被覆盖因为{4, 5}在第二个k组中
        {6, 7},    // 不应该被覆盖
        {1, 6}     // Mode A应该被覆盖，Mode C不应该因为{1, 6}不在任何单个k组中
    };

    // 测试Mode A (CoverMinOneS)
    auto resultA = coverageCalc->calculateCoverage(
        k_groups, 
        j_combinations,
        CoverageMode::CoverMinOneS
    );
    printCoverageResult("Mode A (CoverMinOneS)", resultA, k_groups, j_combinations);
    EXPECT_EQ(resultA.covered_j_count, 4);  // 4个j应该被覆盖
    EXPECT_EQ(resultA.total_j_count, 5);
    EXPECT_DOUBLE_EQ(resultA.coverage_ratio, 0.8);
    EXPECT_EQ((resultA.j_coverage_status), (std::vector<bool>{true, true, true, false, true}));

    // 测试Mode B (CoverMinNS)，要求至少2个s被覆盖
    auto resultB = coverageCalc->calculateCoverage(
        k_groups,
        j_combinations,
        CoverageMode::CoverMinNS,
        2
    );
    printCoverageResult("Mode B (CoverMinNS)", resultB, k_groups, j_combinations);
    EXPECT_EQ(resultB.covered_j_count, 2);  // 只有2个j有至少2个s被覆盖
    EXPECT_DOUBLE_EQ(resultB.coverage_ratio, 0.4);

    // 测试Mode C (CoverAllS)
    auto resultC = coverageCalc->calculateCoverage(
        k_groups,
        j_combinations,
        CoverageMode::CoverAllS
    );
    printCoverageResult("Mode C (CoverAllS)", resultC, k_groups, j_combinations);
    
    std::cout << "\nMode C 详细覆盖分析:" << std::endl;
    for (size_t i = 0; i < j_combinations.size(); ++i) {
        std::cout << "组合 " << i << " {";
        for (int elem : j_combinations[i]) {
            std::cout << elem << " ";
        }
        std::cout << "} 分析:" << std::endl;
        std::cout << "  - 组合大小: " << j_combinations[i].size() << std::endl;
        std::cout << "  - 被覆盖元素数: " << resultC.j_covered_s_counts[i] << std::endl;
        std::cout << "  - 覆盖状态: " << (resultC.j_coverage_status[i] ? "是" : "否") << std::endl;
        std::cout << "  - 是否应该被覆盖: " << (j_combinations[i].size() == resultC.j_covered_s_counts[i] ? "是" : "否") << std::endl;
    }
    
    EXPECT_EQ(resultC.covered_j_count, 3);  // {1,2}, {3}, 和 {4,5} 都应该被覆盖，因为它们都在单个k组中
    EXPECT_DOUBLE_EQ(resultC.coverage_ratio, 0.6);
}

// 空集和边界情况测试
TEST_F(CoverageCalculatorTest, EdgeCases) {
    std::vector<std::vector<int>> empty_k_groups = {};
    std::vector<std::vector<int>> empty_j_combinations = {};
    std::vector<std::vector<int>> j_combinations = {{1, 2}, {3, 4}};

    // 测试空k组
    auto result1 = coverageCalc->calculateCoverage(
        empty_k_groups,
        j_combinations,
        CoverageMode::CoverMinOneS
    );
    EXPECT_EQ(result1.coverage_ratio, 0.0);
    EXPECT_EQ(result1.covered_j_count, 0);
    EXPECT_EQ(result1.total_j_count, 2);

    // 测试空j组合
    auto result2 = coverageCalc->calculateCoverage(
        {{1, 2}},
        empty_j_combinations,
        CoverageMode::CoverMinOneS
    );
    EXPECT_EQ(result2.coverage_ratio, 0.0);
    EXPECT_EQ(result2.covered_j_count, 0);
    EXPECT_EQ(result2.total_j_count, 0);

    // 测试单个空集
    auto result3 = coverageCalc->calculateCoverage(
        {{1, 2}},
        {{}},
        CoverageMode::CoverMinOneS
    );
    EXPECT_EQ(result3.coverage_ratio, 0.0);
    EXPECT_EQ(result3.j_covered_s_counts[0], 0);
}

// 大规模数据测试
TEST_F(CoverageCalculatorTest, LargeScaleTest) {
    auto k_groups = generateRandomSets(20, 5);  // 20个k=5的组
    auto j_combinations = generateRandomSets(1000, 3);  // 1000个大小为3的j组合

    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = coverageCalc->calculateCoverage(
        k_groups,
        j_combinations,
        CoverageMode::CoverMinOneS
    );

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 验证结果
    EXPECT_EQ(result.total_j_count, 1000);
    EXPECT_GE(result.coverage_ratio, 0.0);
    EXPECT_LE(result.coverage_ratio, 1.0);
    EXPECT_EQ((result.j_coverage_status.size()), 1000);
    EXPECT_EQ((result.j_covered_s_counts.size()), 1000);

    // 性能要求：处理1000个组合应该在1秒内完成
    EXPECT_LT(duration.count(), 1000);
}

// 并行性能测试
TEST_F(CoverageCalculatorTest, ParallelPerformanceTest) {
    auto k_groups = generateRandomSets(50, 10);  // 50个k=10的组
    auto j_combinations = generateRandomSets(10000, 5);  // 10000个大小为5的j组合

    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = coverageCalc->calculateCoverage(
        k_groups,
        j_combinations,
        CoverageMode::CoverMinOneS
    );

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 验证结果的正确性
    EXPECT_EQ(result.total_j_count, 10000);
    EXPECT_EQ((result.j_coverage_status.size()), 10000);
    EXPECT_EQ((result.j_covered_s_counts.size()), 10000);

    // 性能要求：并行处理应该显著快于串行处理
    EXPECT_LT(duration.count(), 2000);  // 处理10000个组合应该在2秒内完成
} 