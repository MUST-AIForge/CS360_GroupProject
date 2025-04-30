#include <gtest/gtest.h>
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

using namespace core_algo;

class SetOperationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = Config();
        config.enableCache = true;  // 启用缓存进行测试
        setOps = SetOperations::create(config);
        coverageCalc = CoverageCalculator::create(config);
    }

    void TearDown() override {
        setOps.reset();
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
            // 去重并排序
            std::sort(set.begin(), set.end());
            set.erase(std::unique(set.begin(), set.end()), set.end());
        }
        return sets;
    }

    Config config;
    std::shared_ptr<SetOperations> setOps;
    std::unique_ptr<CoverageCalculator> coverageCalc;
};

// 基本集合操作测试
TEST_F(SetOperationsTest, UnionOperation) {
    std::vector<std::vector<int>> sets = {
        {1, 2, 3, 4},
        {3, 4, 5, 6}
    };
    auto result = setOps->getUnion(sets);
    std::vector<int> expected = {1, 2, 3, 4, 5, 6};
    EXPECT_EQ(result, expected);
}

TEST_F(SetOperationsTest, EmptySetUnion) {
    std::vector<std::vector<int>> sets = {};
    auto result = setOps->getUnion(sets);
    EXPECT_TRUE(result.empty());

    sets = {{}, {}};
    result = setOps->getUnion(sets);
    EXPECT_TRUE(result.empty());
}

TEST_F(SetOperationsTest, IntersectionOperation) {
    std::vector<std::vector<int>> sets = {
        {1, 2, 3, 4},
        {3, 4, 5, 6},
        {4, 5, 6, 7}
    };
    auto result = setOps->getIntersection(sets);
    std::vector<int> expected = {4};
    EXPECT_EQ(result, expected);
}

TEST_F(SetOperationsTest, EmptyIntersection) {
    std::vector<std::vector<int>> sets = {
        {1, 2},
        {3, 4},
        {5, 6}
    };
    auto result = setOps->getIntersection(sets);
    EXPECT_TRUE(result.empty());
}

// 缓存功能测试
TEST_F(SetOperationsTest, CacheEffectiveness) {
    auto sets = generateRandomSets(100, 50);  // 生成大量数据
    
    // 第一次调用，测量时间
    auto start = std::chrono::high_resolution_clock::now();
    auto result1 = setOps->getUnion(sets);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 第二次调用，应该使用缓存
    start = std::chrono::high_resolution_clock::now();
    auto result2 = setOps->getUnion(sets);
    end = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 验证结果相同且第二次调用更快
    EXPECT_EQ(result1, result2);
    EXPECT_LT(duration2.count(), duration1.count());
}

// 大规模数据测试
TEST_F(SetOperationsTest, LargeSetOperations) {
    auto largeSets = generateRandomSets(10, 1000);  // 生成大规模数据
    
    // 测试并集操作
    EXPECT_NO_THROW({
        auto result = setOps->getUnion(largeSets);
        EXPECT_FALSE(result.empty());
    });

    // 测试交集操作
    EXPECT_NO_THROW({
        auto result = setOps->getIntersection(largeSets);
    });
}

// 边界情况测试
TEST_F(SetOperationsTest, EdgeCases) {
    // 测试单个集合
    std::vector<std::vector<int>> singleSet = {{1, 2, 3}};
    auto result = setOps->getUnion(singleSet);
    EXPECT_EQ(result, singleSet[0]);

    // 测试包含重复元素的集合
    std::vector<std::vector<int>> duplicateSets = {
        {1, 1, 2, 2, 3},
        {2, 2, 3, 3, 4}
    };
    result = setOps->getUnion(duplicateSets);
    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(result, expected);
}

// 覆盖率计算测试
TEST_F(SetOperationsTest, CoverageCalculation) {
    std::vector<std::vector<int>> universe = {{1, 2, 3, 4, 5}};
    std::vector<std::vector<int>> selectedSets = {{1, 2, 3}};
    double coverage = setOps->calculateCoverage(universe, selectedSets);
    EXPECT_DOUBLE_EQ(coverage, 0.6);  // 3/5 = 0.6

    // 测试完全覆盖
    selectedSets = {{1, 2, 3, 4, 5}};
    coverage = setOps->calculateCoverage(universe, selectedSets);
    EXPECT_DOUBLE_EQ(coverage, 1.0);

    // 测试零覆盖
    selectedSets = {{6, 7, 8}};
    coverage = setOps->calculateCoverage(universe, selectedSets);
    EXPECT_DOUBLE_EQ(coverage, 0.0);
}

// 组合生成测试
TEST_F(SetOperationsTest, CombinationGeneration) {
    std::vector<std::vector<int>> sets = {
        {1, 2},
        {2, 3}
    };
    auto combinations = setOps->getAllCombinations(sets);
    EXPECT_FALSE(combinations.empty());

    // 测试空集合
    sets = {};
    combinations = setOps->getAllCombinations(sets);
    EXPECT_TRUE(combinations.empty());
}

// 集合验证测试
TEST_F(SetOperationsTest, SetValidation) {
    std::vector<int> validSet = {1, 2, 3, 4};
    EXPECT_TRUE(setOps->isValid(validSet));

    // 测试空集
    std::vector<int> emptySet = {};
    EXPECT_FALSE(setOps->isValid(emptySet));

    // 测试包含重复元素的集合
    std::vector<int> duplicateSet = {1, 1, 2, 2};
    EXPECT_FALSE(setOps->isValid(duplicateSet));

    // 测试包含负数的集合
    std::vector<int> negativeSet = {-1, 1, 2};
    EXPECT_FALSE(setOps->isValid(negativeSet));
}

// 集合标准化测试
TEST_F(SetOperationsTest, SetNormalization) {
    std::vector<int> messySet = {3, 1, 2, 2, 1, 4, -1};
    auto normalizedSet = setOps->normalize(messySet);
    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(normalizedSet, expected);

    // 测试空集
    std::vector<int> emptySet = {};
    auto normalizedEmpty = setOps->normalize(emptySet);
    EXPECT_TRUE(normalizedEmpty.empty());
}

// 性能测试
TEST_F(SetOperationsTest, PerformanceTest) {
    auto sets = generateRandomSets(100, 1000);  // 生成大规模数据
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = setOps->getUnion(sets);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 确保性能在可接受范围内（例如小于1秒）
    EXPECT_LT(duration.count(), 1000);
}

// Jaccard相似度测试
TEST_F(SetOperationsTest, JaccardSimilarity) {
    std::vector<int> setA = {1, 2, 3, 4};
    std::vector<int> setB = {3, 4, 5, 6};
    double similarity = setOps->calculateJaccardSimilarity(setA, setB);
    EXPECT_DOUBLE_EQ(similarity, 2.0 / 6.0);

    // 测试相同集合
    similarity = setOps->calculateJaccardSimilarity(setA, setA);
    EXPECT_DOUBLE_EQ(similarity, 1.0);

    // 测试完全不同的集合
    std::vector<int> setC = {5, 6, 7, 8};
    similarity = setOps->calculateJaccardSimilarity(setA, setC);
    EXPECT_DOUBLE_EQ(similarity, 0.0);

    // 测试空集
    std::vector<int> emptySet = {};
    similarity = setOps->calculateJaccardSimilarity(emptySet, emptySet);
    EXPECT_DOUBLE_EQ(similarity, 1.0);
    similarity = setOps->calculateJaccardSimilarity(setA, emptySet);
    EXPECT_DOUBLE_EQ(similarity, 0.0);
} 