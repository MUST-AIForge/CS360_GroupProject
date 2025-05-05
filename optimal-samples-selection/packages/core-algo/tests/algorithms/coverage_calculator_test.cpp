#include <gtest/gtest.h>
#include "coverage_calculator.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "types.hpp"
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <set>

using namespace core_algo;

// 生成大小为s的子集
void generateSubsets(const std::vector<int>& input, int s, std::vector<std::vector<int>>& result) {
    std::vector<int> subset;
    std::vector<bool> v(input.size());
    std::fill(v.begin(), v.begin() + s, true);

    do {
        subset.clear();
        for (size_t i = 0; i < input.size(); ++i) {
            if (v[i]) {
                subset.push_back(input[i]);
            }
        }
        result.push_back(subset);
    } while (std::prev_permutation(v.begin(), v.end()));
}

class CoverageCalculatorTest : public ::testing::Test {
protected:
    Config config;
    std::unique_ptr<CoverageCalculator> coverageCalc;
    std::shared_ptr<SetOperations> setOps;

    void SetUp() override {
        config.enableRandomization = true;
        std::random_device rd;
        config.randomSeed = rd();
        coverageCalc = CoverageCalculator::create(config);
        setOps = SetOperations::create(config);
    }

    void TearDown() override {
        coverageCalc.reset();
        setOps.reset();
    }

    // 优化的随机测试数据生成函数
    std::pair<std::vector<std::vector<int>>, std::vector<std::vector<int>>> generateRandomSets(
        int k_count, int k_size, int j_count, int j_size) {
        std::random_device rd;
        std::mt19937 gen(rd());

        // 创建基础元素池
        int max_size = std::max(k_size, j_size);
        std::vector<int> base_pool(max_size * 3);  // 使用更大的池以确保有足够的元素
        std::iota(base_pool.begin(), base_pool.end(), 1);

        // 生成k组合
        std::vector<std::vector<int>> k_groups;
        k_groups.reserve(k_count);
        std::vector<int> temp_pool = base_pool;  // 用于随机选择的临时池

        for (int i = 0; i < k_count; ++i) {
            std::vector<int> group;
            group.reserve(k_size);
            
            // 随机打乱临时池并取前k_size个元素
            std::shuffle(temp_pool.begin(), temp_pool.end(), gen);
            group.assign(temp_pool.begin(), temp_pool.begin() + k_size);
            std::sort(group.begin(), group.end());  // 可选：保持有序
            k_groups.push_back(group);
            
            // 重置临时池
            temp_pool = base_pool;
        }

        // 生成j组合
        std::vector<std::vector<int>> j_groups;
        j_groups.reserve(j_count);
        temp_pool = base_pool;  // 重置临时池

        for (int i = 0; i < j_count; ++i) {
            std::vector<int> group;
            group.reserve(j_size);
            
            // 随机打乱临时池并取前j_size个元素
            std::shuffle(temp_pool.begin(), temp_pool.end(), gen);
            group.assign(temp_pool.begin(), temp_pool.begin() + j_size);
            std::sort(group.begin(), group.end());  // 可选：保持有序
            j_groups.push_back(group);
            
            // 重置临时池
            temp_pool = base_pool;
        }

        return {k_groups, j_groups};
    }

    // 生成有序的样本空间
    std::vector<int> generateSampleSpace(int m) {
        std::vector<int> samples(m);
        std::iota(samples.begin(), samples.end(), 1);  // 生成1到m的连续数字
        return samples;
    }

    void printCoverageResult(const std::string& mode_name, const CoverageResult& result, 
                           const std::vector<std::vector<int>>& k_groups,
                           const std::vector<std::vector<int>>& j_groups,
                           int s = 3) {
        std::cout << "\n=== " << mode_name << " 测试结果 ===" << std::endl;
        
        // 打印k组信息
        std::cout << "k_groups (选中的组合):" << std::endl;
        for (size_t i = 0; i < k_groups.size(); ++i) {
            std::cout << "  组 " << i << ": {";
            for (int elem : k_groups[i]) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }

        // 打印j组合信息
        std::cout << "\nj_groups (待覆盖的组合):" << std::endl;
        for (size_t i = 0; i < j_groups.size(); ++i) {
            std::cout << "  组 " << i << ": {";
            for (int elem : j_groups[i]) {
                std::cout << elem << " ";
            }
            std::cout << "} - " << (result.j_coverage_status[i] ? "已覆盖" : "未覆盖");
            std::cout << " (覆盖的S子集数: " << result.j_covered_s_counts[i] << ")" << std::endl;
        }

        std::cout << "\n覆盖率: " << result.coverage_ratio;
        std::cout << "\n覆盖的j组数量: " << result.covered_j_count << "/" << result.total_j_count;
        std::cout << "\n总组数: " << result.total_groups << std::endl;
    }

    void printTestData(const std::vector<std::vector<int>>& k_groups, 
                      const std::vector<std::vector<int>>& j_groups) {
        std::cout << "\n生成的数据:" << std::endl;
        std::cout << "k_groups (" << k_groups.size() << " 组):" << std::endl;
        for (size_t i = 0; i < k_groups.size(); ++i) {
            std::cout << "  组 " << i << ": {";
            for (int elem : k_groups[i]) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }

        std::cout << "\nj_groups (" << j_groups.size() << " 组):" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), j_groups.size()); ++i) {
            std::cout << "  组合 " << i << ": {";
            for (int elem : j_groups[i]) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }
        if (j_groups.size() > 5) {
            std::cout << "  ... (仅显示前5组)" << std::endl;
        }
    }

    void testAllModes(const std::vector<std::vector<int>>& k_groups, 
                      const std::vector<std::vector<int>>& j_groups,
                      int s) {
        std::cout << "生成的数据:" << std::endl;
        std::cout << "k_groups (" << k_groups.size() << " 组):" << std::endl;
        for (size_t i = 0; i < k_groups.size(); i++) {
            std::cout << "  组 " << i << ": {";
            for (size_t j = 0; j < k_groups[i].size(); j++) {
                std::cout << k_groups[i][j];
                if (j < k_groups[i].size() - 1) std::cout << " ";
            }
            std::cout << " }" << std::endl;
        }

        std::cout << "\nj_groups (" << j_groups.size() << " 组):" << std::endl;
        for (size_t i = 0; i < std::min(size_t(5), j_groups.size()); i++) {
            std::cout << "  组合 " << i << ": {";
            for (size_t j = 0; j < j_groups[i].size(); j++) {
                std::cout << j_groups[i][j];
                if (j < j_groups[i].size() - 1) std::cout << " ";
            }
            std::cout << " }" << std::endl;
        }
        if (j_groups.size() > 5) {
            std::cout << "  ... (仅显示前5组)" << std::endl;
        }

        // 生成s子集
        auto generator = CombinationGenerator::create();
        std::vector<std::vector<std::vector<int>>> s_subsets;
        for (const auto& j_group : j_groups) {
            s_subsets.push_back(generator->generate(j_group, s));
        }

        // 测试所有模式
        std::vector<CoverageMode> modes = {CoverageMode::CoverMinOneS, CoverageMode::CoverMinNS, CoverageMode::CoverAllS};
        std::vector<std::string> modeNames = {"A", "B", "C"};
        std::vector<std::string> modeFullNames = {"CoverMinOneS", "CoverMinNS", "CoverAllS"};

        for (size_t i = 0; i < modes.size(); i++) {
            auto mode = modes[i];
            auto startTime = std::chrono::high_resolution_clock::now();
            auto result = coverageCalc->calculateCoverage(k_groups, j_groups, s_subsets, mode, 2);
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

            std::cout << "Mode " << modeNames[i] << " 覆盖计算 耗时: " << duration.count() / 1000.0 << "ms" << std::endl;
            std::cout << "Mode " << modeNames[i] << " (" << modeFullNames[i] << ") 结果:" << std::endl;
            std::cout << "处理时间: " << duration.count() / 1000.0 << "ms" << std::endl;
            std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
            std::cout << "覆盖数量: " << result.covered_j_count << "/" << j_groups.size() << std::endl;

            if (result.coverage_ratio == 0) {
                std::cout << "\n覆盖率为0的详细分析:" << std::endl;
                for (size_t j = 0; j < std::min(size_t(3), j_groups.size()); j++) {
                    std::cout << "  组合 " << j << " {";
                    for (size_t k = 0; k < j_groups[j].size(); k++) {
                        std::cout << j_groups[j][k];
                        if (k < j_groups[j].size() - 1) std::cout << " ";
                    }
                    std::cout << " } - 覆盖状态: 否, 覆盖元素数: 0" << std::endl;

                    std::cout << "    子集分析 (s=" << s << "):" << std::endl;
                    std::vector<std::vector<int>> subsets;
                    generateSubsets(j_groups[j], s, subsets);
                    for (const auto& subset : subsets) {
                        std::cout << "    - {";
                        for (size_t k = 0; k < subset.size(); k++) {
                            std::cout << subset[k];
                            if (k < subset.size() - 1) std::cout << " ";
                        }
                        std::cout << " } 被覆盖情况: 未被任何k组覆盖" << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
    }
};

// 基本覆盖率计算测试
TEST_F(CoverageCalculatorTest, BasicCoverageCalculation) {
    std::cout << "\n=== 开始基本覆盖率计算测试 ===" << std::endl;
    
    // k组：每组3个元素
    std::vector<std::vector<int>> k_groups = {
        {1, 2, 3},  // 可以覆盖 {1,2}, {2,3}
        {3, 4, 5}   // 可以覆盖 {3,4}, {4,5}
    };

    // j组：每组3个元素，包含一些不应该被覆盖的组
    std::vector<std::vector<int>> j_groups = {
        {1, 2, 3},  // 应该被覆盖（被第一个k组覆盖）
        {2, 3, 4},  // 应该被覆盖（被第一个和第二个k组部分覆盖）
        {3, 4, 5},  // 应该被覆盖（被第二个k组覆盖）
        {4, 5, 6},  // 不应该被完全覆盖（6不在任何k组中）
        {1, 3, 5}   // 不应该被完全覆盖（没有k组同时包含1和5）
    };

    std::cout << "\nk_groups (k组):" << std::endl;
    for (size_t i = 0; i < k_groups.size(); i++) {
        std::cout << "  组 " << i << ": {";
        for (int num : k_groups[i]) {
            std::cout << num << " ";
        }
        std::cout << "}" << std::endl;
    }

    std::cout << "\nj_groups (j组):" << std::endl;
    for (size_t i = 0; i < j_groups.size(); i++) {
        std::cout << "  组 " << i << ": {";
        for (int num : j_groups[i]) {
            std::cout << num << " ";
        }
        std::cout << "}" << std::endl;
    }

    // 生成s子集（s=2，这样每个j组会有多个s子集）
    std::vector<std::vector<std::vector<int>>> s_subsets;
    auto generator = CombinationGenerator::create();
    for (const auto& j_group : j_groups) {
        s_subsets.push_back(generator->generate(j_group, 2));  // 使用 CombinationGenerator 生成 s 子集
        
        // 打印每个j组的s子集
        std::cout << "\nj组 {";
        for (int num : j_group) {
            std::cout << num << " ";
        }
        std::cout << "} 的s子集:" << std::endl;
        for (const auto& subset : s_subsets.back()) {
            std::cout << "  {";
            for (int num : subset) {
                std::cout << num << " ";
            }
            std::cout << "}" << std::endl;
        }
    }

    // 测试所有模式
    std::vector<CoverageMode> modes = {CoverageMode::CoverMinOneS, CoverageMode::CoverMinNS, CoverageMode::CoverAllS};
    std::vector<std::string> modeNames = {"A", "B", "C"};
    std::vector<std::string> modeFullNames = {"CoverMinOneS", "CoverMinNS", "CoverAllS"};

    for (size_t i = 0; i < modes.size(); i++) {
        auto mode = modes[i];
        auto startTime = std::chrono::high_resolution_clock::now();
        auto result = coverageCalc->calculateCoverage(k_groups, j_groups, s_subsets, mode, 1);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        std::cout << "\nMode " << modeNames[i] << " 覆盖计算 耗时: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "Mode " << modeNames[i] << " (" << modeFullNames[i] << ") 结果:" << std::endl;
        std::cout << "处理时间: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
        std::cout << "覆盖数量: " << result.covered_j_count << "/" << j_groups.size() << std::endl;

        // 打印每个j组的覆盖情况
        std::cout << "\n详细覆盖情况:" << std::endl;
        for (size_t j = 0; j < j_groups.size(); j++) {
            std::cout << "j组 " << j << " {";
            for (int num : j_groups[j]) {
                std::cout << num << " ";
            }
            std::cout << "} - 覆盖状态: " << (result.j_coverage_status[j] ? "是" : "否")
                     << ", 覆盖的s子集数: " << result.j_covered_s_counts[j] << std::endl;

            std::cout << "  s子集覆盖分析:" << std::endl;
            for (const auto& subset : s_subsets[j]) {
                std::cout << "  - 子集 {";
                for (int num : subset) {
                    std::cout << num << " ";
                }
                std::cout << "} 被覆盖情况: ";
                bool covered = false;
                for (const auto& k_group : k_groups) {
                    if (setOps->contains(k_group, subset)) {
                        std::cout << "被k组 {";
                        for (int num : k_group) {
                            std::cout << num << " ";
                        }
                        std::cout << "} 覆盖";
                        covered = true;
                        break;
                    }
                }
                if (!covered) {
                    std::cout << "未被任何k组覆盖";
                }
                std::cout << std::endl;
            }
        }

        // 修改期望值
        if (mode == CoverageMode::CoverMinOneS) {
            EXPECT_EQ(result.covered_j_count, 3) << "Mode A: 覆盖的j组数量不正确";
            EXPECT_DOUBLE_EQ(result.coverage_ratio, 0.6) << "Mode A: 覆盖率不正确";
        } else if (mode == CoverageMode::CoverMinNS) {
            EXPECT_EQ(result.covered_j_count, 3) << "Mode B: 覆盖的j组数量不正确";
            EXPECT_DOUBLE_EQ(result.coverage_ratio, 0.6) << "Mode B: 覆盖率不正确";
        } else {  // CoverAllS
            EXPECT_EQ(result.covered_j_count, 3) << "Mode C: 覆盖的j组数量不正确";
            EXPECT_DOUBLE_EQ(result.coverage_ratio, 0.6) << "Mode C: 覆盖率不正确";
        }
    }
    
    std::cout << "\n基本覆盖率计算测试完成" << std::endl;
}

// 空集和边界情况测试
TEST_F(CoverageCalculatorTest, EdgeCases) {
    std::vector<std::vector<int>> empty_k_groups = {};
    std::vector<std::vector<int>> empty_j_groups = {};
    std::vector<std::vector<int>> j_groups = {{1, 2}, {3, 4}};

    // 生成s子集
    auto generator = CombinationGenerator::create();
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_group : j_groups) {
        s_subsets.push_back(generator->generate(j_group, 2));
    }

    std::vector<std::vector<std::vector<int>>> empty_s_subsets;

    // 测试空k组
    auto result1 = coverageCalc->calculateCoverage(
        empty_k_groups,
        j_groups,
        s_subsets,
        CoverageMode::CoverMinOneS,
        1
    );
    EXPECT_EQ(result1.coverage_ratio, 0.0);
    EXPECT_EQ(result1.covered_j_count, 0);
    EXPECT_EQ(result1.total_j_count, 2);

    // 测试空j组
    auto result2 = coverageCalc->calculateCoverage(
        {{1, 2}},
        empty_j_groups,
        empty_s_subsets,
        CoverageMode::CoverMinOneS,
        1
    );
    EXPECT_EQ(result2.coverage_ratio, 0.0);
    EXPECT_EQ(result2.covered_j_count, 0);
    EXPECT_EQ(result2.total_j_count, 0);

    // 测试单个空集
    std::vector<std::vector<std::vector<int>>> single_empty_s_subset = {{}};
    auto result3 = coverageCalc->calculateCoverage(
        {{1, 2}},
        {{}},
        single_empty_s_subset,
        CoverageMode::CoverMinOneS,
        1
    );
    EXPECT_EQ(result3.coverage_ratio, 0.0);
    EXPECT_EQ(result3.j_covered_s_counts[0], 0);
}

// 大规模数据测试
TEST_F(CoverageCalculatorTest, LargeScaleTest) {
    std::cout << "\n=== 开始大规模数据测试 ===" << std::endl;
    
    // 设置测试参数
    int k = 7;   // k组的大小
    int j = 7;   // j组的大小
    int s = 5;   // 子集大小
    
    std::cout << "测试参数:" << std::endl;
    std::cout << "k (k组大小) = " << k << std::endl;
    std::cout << "j (j组大小) = " << j << std::endl;
    std::cout << "s (子集大小) = " << s << std::endl;
    
    // 生成测试数据
    int k_count = 10;  // 生成10个k组
    int j_count = 20;  // 生成20个j组
    auto [k_groups, j_groups] = generateRandomSets(k_count, k, j_count, j);
    
    // 生成s子集
    auto generator = CombinationGenerator::create();
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_group : j_groups) {
        s_subsets.push_back(generator->generate(j_group, s));
    }

    std::cout << "\nk_groups 统计:" << std::endl;
    std::cout << "- 组数: " << k_groups.size() << std::endl;
    std::cout << "- 每组大小: " << k << std::endl;
    std::cout << "- 前3组内容:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(3), k_groups.size()); ++i) {
        std::cout << "  组 " << i << ": {";
        for (int num : k_groups[i]) {
            std::cout << num << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    std::cout << "\nj_groups 统计:" << std::endl;
    std::cout << "- 组数: " << j_groups.size() << std::endl;
    std::cout << "- 每组大小: " << j << std::endl;
    std::cout << "- 前3组内容:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(3), j_groups.size()); ++i) {
        std::cout << "  组 " << i << ": {";
        for (int num : j_groups[i]) {
            std::cout << num << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    std::cout << "\n开始计算覆盖率..." << std::endl;
    
    // 测试所有模式
    std::vector<CoverageMode> modes = {CoverageMode::CoverMinOneS, CoverageMode::CoverMinNS, CoverageMode::CoverAllS};
    std::vector<std::string> modeNames = {"A", "B", "C"};
    std::vector<std::string> modeFullNames = {"CoverMinOneS", "CoverMinNS", "CoverAllS"};

    for (size_t i = 0; i < modes.size(); i++) {
        auto mode = modes[i];
        std::cout << "\n计算 Mode " << modeNames[i] << " (" << modeFullNames[i] << ") 覆盖率..." << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        auto result = coverageCalc->calculateCoverage(k_groups, j_groups, s_subsets, mode, 2);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        std::cout << "处理时间: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "总组合数: " << result.total_j_count << std::endl;
        std::cout << "覆盖组合数: " << result.covered_j_count << std::endl;
        std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
        
        // 打印每个j组的覆盖详情
        std::cout << "\n前5个j组的覆盖详情:" << std::endl;
        for (size_t j = 0; j < std::min(size_t(5), result.j_coverage_status.size()); ++j) {
            std::cout << "j组 " << j << ": "
                     << "覆盖状态=" << (result.j_coverage_status[j] ? "是" : "否")
                     << ", 覆盖的s子集数=" << result.j_covered_s_counts[j]
                     << std::endl;
        }
    }
    
    std::cout << "\n大规模数据测试完成" << std::endl;
}

// 并行性能测试
TEST_F(CoverageCalculatorTest, ParallelPerformanceTest) {
    std::cout << "\n=== 开始并行性能测试 ===" << std::endl;
    
    // 设置测试参数
    int k = 7;   // k组的大小
    int j = 7;   // j组的大小
    int s = 5;   // 子集大小
    
    std::cout << "测试参数:" << std::endl;
    std::cout << "k (k组大小) = " << k << std::endl;
    std::cout << "j (j组大小) = " << j << std::endl;
    std::cout << "s (子集大小) = " << s << std::endl;
    
    // 生成大量的k组和j组用于性能测试
    int k_count = 50;    // 生成50个k组
    int j_count = 1000;  // 生成1000个j组
    
    std::cout << "\n生成测试数据..." << std::endl;
    std::cout << "k组数量: " << k_count << std::endl;
    std::cout << "j组数量: " << j_count << std::endl;
    
    auto start_gen = std::chrono::high_resolution_clock::now();
    auto [k_groups, j_groups] = generateRandomSets(k_count, k, j_count, j);
    auto end_gen = std::chrono::high_resolution_clock::now();
    auto duration_gen = std::chrono::duration_cast<std::chrono::milliseconds>(end_gen - start_gen);
    
    std::cout << "数据生成耗时: " << duration_gen.count() << "ms" << std::endl;
    
    // 生成s子集
    auto generator = CombinationGenerator::create();
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_group : j_groups) {
        s_subsets.push_back(generator->generate(j_group, s));
    }

    // 验证生成的数据
    std::cout << "\n验证生成的数据..." << std::endl;
    ASSERT_EQ(k_groups.size(), k_count) << "k组数量不正确";
    ASSERT_EQ(j_groups.size(), j_count) << "j组数量不正确";
    
    for (const auto& group : k_groups) {
        ASSERT_EQ(group.size(), k) << "k组大小不正确";
    }
    for (const auto& group : j_groups) {
        ASSERT_EQ(group.size(), j) << "j组大小不正确";
    }
    
    std::cout << "\n开始并行覆盖计算..." << std::endl;
    
    // 对所有模式进行性能测试
    std::vector<CoverageMode> modes = {CoverageMode::CoverMinOneS, CoverageMode::CoverMinNS, CoverageMode::CoverAllS};
    std::vector<std::string> modeNames = {"A", "B", "C"};
    
    for (size_t i = 0; i < modes.size(); i++) {
        std::cout << "\n测试 Mode " << modeNames[i] << ":" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        auto result = coverageCalc->calculateCoverage(k_groups, j_groups, s_subsets, modes[i], 2);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "处理时间: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "总组合数: " << result.total_j_count << std::endl;
        std::cout << "覆盖组合数: " << result.covered_j_count << std::endl;
        std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
        
        // 验证结果
        EXPECT_EQ(result.total_j_count, j_count);
        EXPECT_EQ(result.j_coverage_status.size(), j_count);
        EXPECT_EQ(result.j_covered_s_counts.size(), j_count);
        
        // 输出前几个组的覆盖详情
        std::cout << "\n前3个j组的覆盖详情:" << std::endl;
        for (int j = 0; j < std::min(3, j_count); ++j) {
            std::cout << "j组 " << j << ": "
                     << "覆盖状态=" << (result.j_coverage_status[j] ? "是" : "否")
                     << ", 覆盖的s子集数=" << result.j_covered_s_counts[j]
                     << std::endl;
        }
    }
    
    std::cout << "\n并行性能测试完成" << std::endl;
}

// 大规模参数组合测试
TEST_F(CoverageCalculatorTest, LargeScaleParameterTest) {
    // 测试j=7的情况，s从4到7
    for (int s = 4; s <= 7; s++) {
        std::cout << "\n=== 测试场景: j=7测试 (s=" << s << ") ===" << std::endl << std::endl;
        auto [k_groups, j_groups] = generateRandomSets(10, 7, 20, 7);  // 生成10个k组和20个j组
        testAllModes(k_groups, j_groups, s);
    }

    // 测试k>j的情况
    std::vector<std::tuple<int, int>> kjPairs = {
        {7, 5}, {7, 6}  // 测试k=7时j分别为5和6的情况
    };

    for (const auto& [k, j] : kjPairs) {
        std::cout << "\n=== 测试场景: k>j测试 (k=" << k << ", j=" << j << ") ===" << std::endl << std::endl;
        auto [k_groups, j_groups] = generateRandomSets(10, k, 20, j);  // 生成10个k组和20个j组
        testAllModes(k_groups, j_groups, j-1);  // 使用j-1作为s值
    }
}

// 高重合度大规模测试
TEST_F(CoverageCalculatorTest, HighOverlapTest) {
    std::cout << "\n=== 开始高重合度大规模测试 ===" << std::endl;
    
    // 设置较大的测试参数
    int k = 15;   // k组的大小
    int j = 12;   // j组的大小
    int s = 8;    // 子集大小
    
    std::cout << "测试参数:" << std::endl;
    std::cout << "k (k组大小) = " << k << std::endl;
    std::cout << "j (j组大小) = " << j << std::endl;
    std::cout << "s (子集大小) = " << s << std::endl;
    
    // 手动生成高重合度的k组和j组
    std::vector<std::vector<int>> k_groups;
    std::vector<std::vector<int>> j_groups;
    
    // 生成基础集合
    std::vector<int> base_elements = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
    
    // 生成20个k组，每组15个元素，保证95%的重合度
    int k_count = 20;
    std::cout << "\n生成k组 (目标重合度95%):" << std::endl;
    std::vector<int> k_base(base_elements.begin(), base_elements.begin() + k);  // 基础k组
    
    for (int i = 0; i < k_count; ++i) {
        std::vector<int> group = k_base;  // 复制基础组
        // 随机替换5%的元素
        int change_count = std::max(1, (int)(k * 0.05));  // 至少改变1个元素
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, k-1);  // 选择要替换的位置
        std::uniform_int_distribution<> elem_dis(k+1, base_elements.size());  // 选择新元素
        
        for (int j = 0; j < change_count; ++j) {
            int pos = dis(gen);
            int new_elem = base_elements[elem_dis(gen)-1];
            group[pos] = new_elem;
        }
        std::sort(group.begin(), group.end());
        k_groups.push_back(group);
        
        // 打印前5个k组的详细信息
        if (i < 5) {
            std::cout << "k组 " << i << ": {";
            for (int elem : group) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }
    }
    
    // 生成30个j组，每组12个元素，同样保证95%的重合度
    int j_count = 30;
    std::cout << "\n生成j组 (目标重合度95%):" << std::endl;
    std::vector<int> j_base(k_base.begin(), k_base.begin() + j);  // 基础j组
    
    for (int i = 0; i < j_count; ++i) {
        std::vector<int> group = j_base;  // 复制基础组
        // 随机替换5%的元素
        int change_count = std::max(1, (int)(j * 0.05));  // 至少改变1个元素
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, j-1);  // 选择要替换的位置
        std::uniform_int_distribution<> elem_dis(j+1, base_elements.size());  // 选择新元素
        
        for (int c = 0; c < change_count; ++c) {
            int pos = dis(gen);
            int new_elem = base_elements[elem_dis(gen)-1];
            group[pos] = new_elem;
        }
        std::sort(group.begin(), group.end());
        j_groups.push_back(group);
        
        // 打印前5个j组的详细信息
        if (i < 5) {
            std::cout << "j组 " << i << ": {";
            for (int elem : group) {
                std::cout << elem << " ";
            }
            std::cout << "}" << std::endl;
        }
    }
    
    // 计算实际重合度
    auto calculate_overlap = [](const std::vector<std::vector<int>>& groups) {
        double total_overlap = 0;
        int count = 0;
        for (size_t i = 0; i < groups.size(); ++i) {
            for (size_t j = i + 1; j < groups.size(); ++j) {
                std::vector<int> intersection;
                std::set_intersection(
                    groups[i].begin(), groups[i].end(),
                    groups[j].begin(), groups[j].end(),
                    std::back_inserter(intersection)
                );
                total_overlap += static_cast<double>(intersection.size()) / groups[i].size();
                count++;
            }
        }
        return total_overlap / count;
    };
    
    double k_overlap = calculate_overlap(k_groups);
    double j_overlap = calculate_overlap(j_groups);
    
    std::cout << "\n实际重合度统计:" << std::endl;
    std::cout << "k组平均重合度: " << (k_overlap * 100) << "%" << std::endl;
    std::cout << "j组平均重合度: " << (j_overlap * 100) << "%" << std::endl;
    
    std::cout << "\n开始计算覆盖率..." << std::endl;
    
    // 生成s子集
    auto generator = CombinationGenerator::create();
    std::vector<std::vector<std::vector<int>>> s_subsets;
    for (const auto& j_group : j_groups) {
        s_subsets.push_back(generator->generate(j_group, s));
    }

    // 测试所有模式
    std::vector<CoverageMode> modes = {CoverageMode::CoverMinOneS, CoverageMode::CoverMinNS, CoverageMode::CoverAllS};
    std::vector<std::string> modeNames = {"A", "B", "C"};
    std::vector<std::string> modeFullNames = {"CoverMinOneS", "CoverMinNS", "CoverAllS"};

    for (size_t i = 0; i < modes.size(); i++) {
        auto mode = modes[i];
        auto startTime = std::chrono::high_resolution_clock::now();
        auto result = coverageCalc->calculateCoverage(k_groups, j_groups, s_subsets, mode, 2);
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        std::cout << "Mode " << modeNames[i] << " 覆盖计算 耗时: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "Mode " << modeNames[i] << " (" << modeFullNames[i] << ") 结果:" << std::endl;
        std::cout << "处理时间: " << duration.count() / 1000.0 << "ms" << std::endl;
        std::cout << "覆盖率: " << result.coverage_ratio << std::endl;
        std::cout << "覆盖数量: " << result.covered_j_count << "/" << j_groups.size() << std::endl;

        if (result.coverage_ratio == 0) {
            std::cout << "\n覆盖率为0的详细分析:" << std::endl;
            for (size_t j = 0; j < std::min(size_t(3), j_groups.size()); j++) {
                std::cout << "  组合 " << j << " {";
                for (size_t k = 0; k < j_groups[j].size(); k++) {
                    std::cout << j_groups[j][k];
                    if (k < j_groups[j].size() - 1) std::cout << " ";
                }
                std::cout << " } - 覆盖状态: 否, 覆盖元素数: 0" << std::endl;

                std::cout << "    子集分析 (s=" << s << "):" << std::endl;
                std::vector<std::vector<int>> subsets;
                generateSubsets(j_groups[j], s, subsets);
                for (const auto& subset : subsets) {
                    std::cout << "    - {";
                    for (size_t k = 0; k < subset.size(); k++) {
                        std::cout << subset[k];
                        if (k < subset.size() - 1) std::cout << " ";
                    }
                    std::cout << " } 被覆盖情况: 未被任何k组覆盖" << std::endl;
                }
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n高重合度大规模测试完成" << std::endl;
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 