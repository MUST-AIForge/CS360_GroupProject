#include <gtest/gtest.h>
#include "combination_generator.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <random>
#include <set>
#include <algorithm>

using namespace core_algo;
using namespace std::chrono;

// 添加 GTest main 函数
int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class CombinationGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = Config();
        generator = CombinationGenerator::create(config);
    }

    void TearDown() override {
        generator.reset();
    }

    // 辅助函数：测量执行时间
    template<typename Func>
    double measureTime(Func&& func) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - start).count() / 1000.0;
    }

    // 辅助函数：生成指定范围内的随机数
    int getRandomInRange(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }

    // 辅助函数：生成样本空间
    std::vector<int> generateSampleSpace(int m) {
        std::vector<int> samples(m);
        std::iota(samples.begin(), samples.end(), 1);
        return samples;
    }

    // 辅助函数：从样本空间中选择n个样本
    std::vector<int> selectNSamples(const std::vector<int>& sampleSpace, int n) {
        std::vector<int> selected = sampleSpace;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::shuffle(selected.begin(), selected.end(), gen);
        selected.resize(n);
        std::sort(selected.begin(), selected.end());
        return selected;
    }

    Config config;
    std::unique_ptr<CombinationGenerator> generator;
};

// 基本功能测试：验证j组生成
TEST_F(CombinationGeneratorTest, GenerateJGroups) {
    // 生成符合范围的随机参数
    int m = getRandomInRange(45, 54);
    int n = getRandomInRange(7, std::min(25, m));
    int j = getRandomInRange(4, 7);
    
    std::cout << "\n=== 测试j组生成 ===" << std::endl;
    std::cout << "参数: m=" << m << ", n=" << n << ", j=" << j << std::endl;
    
    // 生成样本空间并选择n个样本
    auto sampleSpace = generateSampleSpace(m);
    auto selectedSamples = selectNSamples(sampleSpace, n);
    
    // 生成j组
    auto jGroups = generator->generate(selectedSamples, j);
    
    // 验证
    std::cout << "生成的j组数量: " << jGroups.size() << std::endl;
    std::cout << "前5个j组示例:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), jGroups.size()); ++i) {
        std::cout << "  组 " << i << ": {";
        for (int elem : jGroups[i]) {
            std::cout << elem << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    // 验证每个j组的大小
    for (const auto& group : jGroups) {
        EXPECT_EQ(group.size(), j) << "j组大小不正确";
        // 验证元素都在选定的样本中
        for (int elem : group) {
            EXPECT_TRUE(std::find(selectedSamples.begin(), selectedSamples.end(), elem) 
                       != selectedSamples.end()) << "j组包含未选定的样本";
        }
    }
}

// 测试s子集生成
TEST_F(CombinationGeneratorTest, GenerateSSubsets) {
    std::cout << "\n=== 测试s子集生成 ===" << std::endl;
    
    // 测试所有合法的j和s组合
    std::vector<std::pair<int, std::vector<int>>> jsCombinations = {
        {4, {3, 4}},
        {5, {3, 4, 5}},
        {6, {3, 4, 5, 6}},
        {7, {3, 4, 5, 6, 7}}
    };
    
    for (const auto& [j, validS] : jsCombinations) {
        std::cout << "\n测试 j=" << j << " 的所有合法s值" << std::endl;
        
        // 生成一个j组作为输入
        std::vector<int> jGroup(j);
        std::iota(jGroup.begin(), jGroup.end(), 1);
        
        for (int s : validS) {
            std::cout << "\n  生成 s=" << s << " 的子集" << std::endl;
            
            auto sSubsets = generator->generate(jGroup, s);
            
            std::cout << "  生成的s子集数量: " << sSubsets.size() << std::endl;
            std::cout << "  前3个s子集示例:" << std::endl;
            for (size_t i = 0; i < std::min(size_t(3), sSubsets.size()); ++i) {
                std::cout << "    子集 " << i << ": {";
                for (int elem : sSubsets[i]) {
                    std::cout << elem << " ";
                }
                std::cout << "}" << std::endl;
            }
            
            // 验证
            for (const auto& subset : sSubsets) {
                // 验证子集大小
                EXPECT_EQ(subset.size(), s) << "s子集大小不正确";
                // 验证子集元素都在j组中
                for (int elem : subset) {
                    EXPECT_TRUE(std::find(jGroup.begin(), jGroup.end(), elem) 
                               != jGroup.end()) << "s子集包含不在j组中的元素";
                }
            }
        }
    }
}

// 性能测试：实际场景
TEST_F(CombinationGeneratorTest, RealWorldPerformance) {
    std::cout << "\n=== 实际场景性能测试 ===" << std::endl;
    
    // 测试不同规模的输入
    std::vector<std::tuple<int, int, int, int>> testCases = {
        // m, n, j, s
        {45, 7, 4, 3},    // 最小规模
        {50, 15, 5, 4},   // 中等规模
        {54, 25, 7, 5}    // 最大规模
    };
    
    for (const auto& testCase : testCases) {
        int m = std::get<0>(testCase);
        int n = std::get<1>(testCase);
        int j = std::get<2>(testCase);
        int s = std::get<3>(testCase);
        
        std::cout << "\n测试规模: m=" << m << ", n=" << n 
                  << ", j=" << j << ", s=" << s << std::endl;
        
        // 生成样本空间和选定样本
        auto sampleSpace = generateSampleSpace(m);
        auto selectedSamples = selectNSamples(sampleSpace, n);
        
        // 测试j组生成性能
        double jGroupTime = measureTime([&]() {
            auto jGroups = generator->generate(selectedSamples, j);
            std::cout << "生成的j组数量: " << jGroups.size() << std::endl;
            
            // 为每个j组生成s子集
            for (const auto& jGroup : jGroups) {
                auto sSubsets = generator->generate(jGroup, s);
            }
        });
        
        std::cout << "总执行时间: " << jGroupTime << "ms" << std::endl;
    }
}

// 边界情况测试
TEST_F(CombinationGeneratorTest, EdgeCases) {
    std::cout << "\n=== 边界情况测试 ===" << std::endl;
    
    // 测试最小和最大参数组合
    std::vector<std::tuple<int, int, int>> testCases = {
        // m, n, j
        {45, 7, 4},    // 最小值
        {54, 25, 7},   // 最大值
        {50, 50, 7},   // n = m 的情况
        {45, 7, 7}     // j = n 的情况
    };
    
    for (const auto& [m, n, j] : testCases) {
        std::cout << "\n测试边界情况: m=" << m << ", n=" << n << ", j=" << j << std::endl;
        
        auto sampleSpace = generateSampleSpace(m);
        auto selectedSamples = selectNSamples(sampleSpace, std::min(n, m));
        
        auto jGroups = generator->generate(selectedSamples, j);
        
        std::cout << "生成的j组数量: " << jGroups.size() << std::endl;
        if (!jGroups.empty()) {
            std::cout << "第一个j组: {";
            for (int elem : jGroups[0]) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }
    }
}

// 测试随机样本生成
TEST_F(CombinationGeneratorTest, GenerateRandomSamples) {
    std::cout << "\n=== 测试随机样本生成 ===" << std::endl;
    
    // 测试用例：不同的 m 和 n 组合
    std::vector<std::pair<int, int>> testCases = {
        {45, 7},   // 最小规模
        {50, 15},  // 中等规模
        {54, 25},  // 最大规模
        {10, 10},  // m = n 的情况
        {20, 5}    // 一般情况
    };
    
    for (const auto& [m, n] : testCases) {
        std::cout << "\n测试参数: m=" << m << ", n=" << n << std::endl;
        
        // 生成随机样本
        auto samples = generator->generateRandomSamples(m, n);
        
        // 验证样本数量
        EXPECT_EQ(samples.size(), n) << "样本数量不正确";
        
        // 验证样本范围
        for (int sample : samples) {
            EXPECT_GE(sample, 1) << "样本值小于1";
            EXPECT_LE(sample, m) << "样本值大于m";
        }
        
        // 验证样本唯一性
        std::set<int> uniqueSamples(samples.begin(), samples.end());
        EXPECT_EQ(uniqueSamples.size(), samples.size()) << "存在重复样本";
        
        // 验证样本是否已排序
        EXPECT_TRUE(std::is_sorted(samples.begin(), samples.end())) << "样本未排序";
        
        std::cout << "生成的样本: {";
        for (int sample : samples) {
            std::cout << sample << " ";
        }
        std::cout << "}" << std::endl;
    }
}

// 测试随机样本生成的异常情况
TEST_F(CombinationGeneratorTest, GenerateRandomSamplesExceptions) {
    std::cout << "\n=== 测试随机样本生成异常情况 ===" << std::endl;
    
    // 测试 n > m 的情况
    EXPECT_THROW({
        generator->generateRandomSamples(5, 10);
    }, std::invalid_argument) << "当 n > m 时应抛出异常";
    
    // 测试边界值
    EXPECT_NO_THROW({
        auto samples = generator->generateRandomSamples(10, 10);
        EXPECT_EQ(samples.size(), 10) << "边界情况 m=n 时样本数量不正确";
    }) << "边界情况 m=n 应正常执行";
    
    // 测试最小值
    EXPECT_NO_THROW({
        auto samples = generator->generateRandomSamples(1, 1);
        EXPECT_EQ(samples.size(), 1) << "最小值情况样本数量不正确";
        EXPECT_EQ(samples[0], 1) << "最小值情况样本值不正确";
    }) << "最小值情况应正常执行";
} 

// 新增：测试generateSSubsetsForJCombination函数
TEST_F(CombinationGeneratorTest, GenerateSSubsetsForJCombination) {
    std::cout << "\n=== 测试generateSSubsetsForJCombination函数 ===" << std::endl;
    
    // 测试用例1：j=4, s=3
    {
        std::vector<int> j_combination = {1, 2, 3, 4};  // 一个j=4的组合
        int s = 3;
        
        std::cout << "\n测试用例1: j=4, s=3" << std::endl;
        std::cout << "输入j组合: [";
        for (int elem : j_combination) {
            std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
        }
        std::cout << "]" << std::endl;
        
        // 计算正确的s子集
        std::vector<std::vector<int>> expected_subsets = {
            {1, 2, 3},
            {1, 2, 4},
            {1, 3, 4},
            {2, 3, 4}
        };
        
        std::cout << "期望的s子集:" << std::endl;
        for (const auto& subset : expected_subsets) {
            std::cout << "  [";
            for (int elem : subset) {
                std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
            }
            std::cout << "]" << std::endl;
        }
        
        // 生成s子集
        auto generated_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证结果
        EXPECT_EQ(generated_subsets.size(), expected_subsets.size()) 
            << "生成的s子集数量不正确";
        
        for (const auto& subset : generated_subsets) {
            // 验证子集大小
            EXPECT_EQ(subset.size(), s) << "s子集大小不正确";
            // 验证子集是否在预期结果中
            bool found = false;
            for (const auto& expected : expected_subsets) {
                if (subset == expected) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << "生成了未预期的s子集";
        }
    }
    
    // 测试用例2：j=5, s=3
    {
        std::vector<int> j_combination = {1, 2, 3, 4, 5};  // 一个j=5的组合
        int s = 3;
        
        std::cout << "\n测试用例2: j=5, s=3" << std::endl;
        std::cout << "输入j组合: [";
        for (int elem : j_combination) {
            std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
        }
        std::cout << "]" << std::endl;
        
        // 计算正确的s子集
        std::vector<std::vector<int>> expected_subsets = {
            {1, 2, 3}, {1, 2, 4}, {1, 2, 5},
            {1, 3, 4}, {1, 3, 5}, {1, 4, 5},
            {2, 3, 4}, {2, 3, 5}, {2, 4, 5},
            {3, 4, 5}
        };
        
        std::cout << "期望的s子集:" << std::endl;
        for (const auto& subset : expected_subsets) {
            std::cout << "  [";
            for (int elem : subset) {
                std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
            }
            std::cout << "]" << std::endl;
        }
        
        // 生成s子集
        auto generated_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证结果
        EXPECT_EQ(generated_subsets.size(), expected_subsets.size()) 
            << "生成的s子集数量不正确";
        
        for (const auto& subset : generated_subsets) {
            // 验证子集大小
            EXPECT_EQ(subset.size(), s) << "s子集大小不正确";
            // 验证子集是否在预期结果中
            bool found = false;
            for (const auto& expected : expected_subsets) {
                if (subset == expected) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << "生成了未预期的s子集";
        }
    }
    
    // 测试用例3：边界情况 j=3, s=3
    {
        std::vector<int> j_combination = {1, 2, 3};  // j=s的情况
        int s = 3;
        
        std::cout << "\n测试用例3: j=3, s=3 (边界情况：j=s)" << std::endl;
        std::cout << "输入j组合: [";
        for (int elem : j_combination) {
            std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
        }
        std::cout << "]" << std::endl;
        
        // 在j=s的情况下，只应该生成一个子集，就是j组合本身
        std::vector<std::vector<int>> expected_subsets = {
            {1, 2, 3}
        };
        
        std::cout << "期望的s子集:" << std::endl;
        for (const auto& subset : expected_subsets) {
            std::cout << "  [";
            for (int elem : subset) {
                std::cout << elem << "(" << static_cast<char>('A' + elem - 1) << ") ";
            }
            std::cout << "]" << std::endl;
        }
        
        // 生成s子集
        auto generated_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证结果
        EXPECT_EQ(generated_subsets.size(), 1) << "边界情况下应该只生成一个子集";
        EXPECT_EQ(generated_subsets[0], j_combination) << "生成的子集应该等于输入的j组合";
    }
}

// 测试s子集生成的正确性
TEST_F(CombinationGeneratorTest, MinimalValidParameters) {
    std::cout << "\n=== 测试最小有效参数的s子集生成 ===" << std::endl;
    
    // 测试用例1：j=4, s=3
    {
        std::vector<int> j_combination = {1, 2, 3, 4};  // 一个4元素的j组合
        int s = 3;
        
        std::cout << "\n测试用例1: j=4, s=3" << std::endl;
        auto s_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证生成的子集数量（应该是4C3 = 4个子集）
        EXPECT_EQ(s_subsets.size(), 4) << "j=4, s=3时子集数量不正确";
        
        // 验证每个子集的大小
        for (const auto& subset : s_subsets) {
            EXPECT_EQ(subset.size(), s) << "子集大小不等于s";
        }
        
        // 验证生成的所有可能子集
        std::vector<std::vector<int>> expected_subsets = {
            {1, 2, 3},
            {1, 2, 4},
            {1, 3, 4},
            {2, 3, 4}
        };
        
        EXPECT_EQ(s_subsets.size(), expected_subsets.size()) << "生成的子集数量与预期不符";
        for (const auto& expected : expected_subsets) {
            bool found = false;
            for (const auto& actual : s_subsets) {
                if (actual == expected) {
                    found = true;
                    break;
                }
            }
            EXPECT_TRUE(found) << "未找到预期的子集";
        }
    }
    
    // 测试用例2：j=5, s=3
    {
        std::vector<int> j_combination = {1, 2, 3, 4, 5};  // 一个5元素的j组合
        int s = 3;
        
        std::cout << "\n测试用例2: j=5, s=3" << std::endl;
        auto s_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证生成的子集数量（应该是5C3 = 10个子集）
        EXPECT_EQ(s_subsets.size(), 10) << "j=5, s=3时子集数量不正确";
        
        // 验证每个子集的大小
        for (const auto& subset : s_subsets) {
            EXPECT_EQ(subset.size(), s) << "子集大小不等于s";
        }
    }
    
    // 测试用例3：边界情况 j=3, s=3
    {
        std::vector<int> j_combination = {1, 2, 3};  // j=s的情况
        int s = 3;
        
        std::cout << "\n测试用例3: j=3, s=3（边界情况）" << std::endl;
        auto s_subsets = generator->generateSSubsetsForJCombination(j_combination, s);
        
        // 验证生成的子集数量（应该只有1个子集）
        EXPECT_EQ(s_subsets.size(), 1) << "j=s=3时子集数量不正确";
        
        // 验证生成的子集
        EXPECT_EQ(s_subsets[0], j_combination) << "j=s时应该只有一个子集，且等于j组合本身";
    }
} 