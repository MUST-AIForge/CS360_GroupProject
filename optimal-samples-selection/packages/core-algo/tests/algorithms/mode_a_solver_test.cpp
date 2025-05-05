#include "mode_a_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include "preprocessor.hpp"
#include "base_solver.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <numeric>
#include <map>
#include <random>
#include <iostream>
#include <set>
#include <thread>

namespace core_algo {
namespace testing {

// 定义组合结果结构
struct CombinationResult {
    std::vector<std::vector<int>> groups;           // k组候选集
    std::vector<std::vector<int>> jCombinations;    // j组集合
    std::vector<std::vector<int>> allSSubsets;      // s子集集合
    std::map<std::vector<int>, std::vector<std::vector<int>>> jToSMap;  // j组到s子集的映射
    std::map<std::vector<int>, std::vector<std::vector<int>>> sToJMap;  // s子集到j组的映射
};

// 定义预处理结果结构
struct PreprocessorResult {
    std::vector<std::vector<int>> selectedSSubsets;  // 选中的s子集
    std::map<std::vector<int>, std::vector<std::vector<int>>> selectedSToJMap;  // 选中的s子集到j组的映射
};

class ModeASetCoverSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_config.threadCount = 4;  // 设置并行测试的线程数
        m_config.useLetter = true; // 启用字母表示
        m_combGen = CombinationGenerator::create(m_config);
        m_setOps = SetOperations::create(m_config);
        m_covCalc = CoverageCalculator::create(m_config);
        m_solver = createModeASolver(m_combGen, m_setOps, m_covCalc, m_config);
        m_preprocessor = std::make_shared<Preprocessor>(m_combGen, m_setOps);
    }

    void TearDown() override {}

    // 辅助函数：将字母转换为数字（A->1, B->2, etc.）
    int letterToNum(char letter) {
        return static_cast<int>(letter - 'A' + 1);
    }

    // 辅助函数：将数字转换为字母（1->A, 2->B, etc.）
    char numToLetter(int num) {
        return static_cast<char>('A' + num - 1);
    }

    // 检查每个jGroup是否至少有一个s子集被覆盖
    bool checkJGroupCoverage(const DetailedSolution& solution, 
                           const std::vector<int>& samples, 
                           int j, int s) {
        if (solution.status != Status::Success) {
            std::cout << "解的状态不是Success" << std::endl;
            return false;
        }

        // 如果使用字母表示，将样本转换为数字
        std::vector<int> numericSamples;
        if (m_config.useLetter) {
            for (int sample : samples) {
                if (sample >= 'A' && sample <= 'Z') {
                    numericSamples.push_back(letterToNum(static_cast<char>(sample)));
                } else {
                    numericSamples.push_back(sample);
                }
            }
        } else {
            numericSamples = samples;
        }

        // 预处理：将所有k组转换为数字表示（如果需要）
        std::vector<std::vector<int>> numericKGroups;
        numericKGroups.reserve(solution.groups.size());
        for (const auto& group : solution.groups) {
            std::vector<int> numericGroup;
            if (m_config.useLetter) {
                for (int elem : group) {
                    if (elem >= 'A' && elem <= 'Z') {
                        numericGroup.push_back(letterToNum(static_cast<char>(elem)));
                    } else {
                        numericGroup.push_back(elem);
                    }
                }
            } else {
                numericGroup = group;
            }
            numericKGroups.push_back(numericGroup);
        }

        auto jGroups = m_combGen->generate(numericSamples, j);
        std::cout << "\n开始验证j组的覆盖情况..." << std::endl;
        std::cout << "总共有 " << jGroups.size() << " 个j组需要验证" << std::endl;
        
        size_t processedGroups = 0;
        const size_t totalGroups = jGroups.size();
        const size_t reportInterval = std::max(size_t(1), totalGroups / 10); // 每10%报告一次进度
        
        // 为每个j组找到其所有s子集
        for (const auto& jGroup : jGroups) {
            processedGroups++;
            if (processedGroups % reportInterval == 0) {
                std::cout << "已处理 " << (processedGroups * 100 / totalGroups) << "% 的j组..." << std::endl;
            }
            
            // 使用generateSSubsetsForJCombination生成s子集
            auto sSubsets = m_combGen->generateSSubsetsForJCombination(jGroup, s);
            bool jGroupCovered = false;
            
            // 检查是否至少有一个s子集被某个k组覆盖
            for (const auto& subset : sSubsets) {
                for (const auto& kGroup : numericKGroups) {
                    if (m_setOps->contains(kGroup, subset)) {
                        jGroupCovered = true;
                        break;
                    }
                }
                if (jGroupCovered) break;
            }
            
            if (!jGroupCovered) {
                std::cout << "\n错误：j组 ";
                for (int elem : jGroup) {
                    if (m_config.useLetter) {
                        std::cout << static_cast<char>('A' + elem - 1) << " ";
                    } else {
                    std::cout << elem << " ";
                    }
                }
                std::cout << "没有任何s子集被k组覆盖" << std::endl;
                return false;
            }
        }
        
        std::cout << "\n所有j组验证完成，覆盖验证成功！" << std::endl;
        return true;
    }

    // 验证解的有效性
    void validateSolution(const DetailedSolution& solution, 
                         const std::vector<int>& samples,
                         int k) {
        // 验证每个k组的大小
        for (const auto& group : solution.groups) {
            EXPECT_EQ(group.size(), k) << "k-group size mismatch";
            
            // 验证k组中的元素都来自samples
            for (int elem : group) {
                EXPECT_NE(std::find(samples.begin(), samples.end(), elem), 
                         samples.end()) 
                    << "Element " << numToLetter(elem) << " not found in samples";
            }
        }
    }

    // 获取生成的样本
    std::vector<int> getGeneratedSamples(int m, int n) {
        return m_combGen->generateRandomSamples(m, n);
    }

    // 从已有解中提取使用的样本
    std::vector<int> extractUsedSamples(const DetailedSolution& solution) {
        std::set<int> uniqueElements;
        for (const auto& group : solution.groups) {
            uniqueElements.insert(group.begin(), group.end());
        }
        return std::vector<int>(uniqueElements.begin(), uniqueElements.end());
    }

    std::shared_ptr<ModeASolver> m_solver;
    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<CoverageCalculator> m_covCalc;
    std::shared_ptr<Preprocessor> m_preprocessor;
    Config m_config;

    // 辅助函数：生成组合
    CombinationResult generateTestCombinations(
        int m, int n,
        const std::vector<int>& samples,
        int k, int s, int j
    ) {
        // 生成所有组合
        CombinationResult result;
        result.groups = m_combGen->generate(samples, k);          // k组候选集
        result.jCombinations = m_combGen->generate(samples, j);   // j组集合
        result.allSSubsets = m_combGen->generate(samples, s);     // s子集集合
        
        // 建立映射关系
        for (const auto& jGroup : result.jCombinations) {
            auto sSubsetsForJ = m_combGen->generate(jGroup, s);
            result.jToSMap[jGroup] = sSubsetsForJ;
            
            for (const auto& sSubset : sSubsetsForJ) {
                result.sToJMap[sSubset].push_back(jGroup);
            }
        }
        
        return result;
    }

// 辅助函数：将数字数组转换为字母表示
std::string groupToLetters(const std::vector<int>& group) {
    std::string result;
    for (int num : group) {
        result += numToLetter(num);
        result += ",";
    }
    if (!result.empty()) {
        result.pop_back(); // 移除最后的逗号
    }
    return result;
}

// 辅助函数：将字母组转换为数字数组
std::vector<int> lettersToGroup(const std::string& letters) {
    std::vector<int> result;
    for (char c : letters) {
        if (c >= 'A' && c <= 'Z') {
            result.push_back(letterToNum(c));
        }
    }
    return result;
    }
};

// 边界值测试：最小有效参数
TEST_F(ModeASetCoverSolverTest, MinimalValidParameters) {
    std::cout << "\n=== 开始 MinimalValidParameters 测试 ===" << std::endl;
    
    int m = 45;  // 最小universeSize
    int n = 7;   // 最小n
    int k = 4;   // 最小k
    int s = 3;   // 最小s
    int j = 3;   // 最小j (等于s)
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", s=" << s << ", j=" << j << std::endl;
    
    // 生成数字样本 1到7
    std::vector<int> samples;
    for (int i = 1; i <= n; ++i) {
        samples.push_back(i);  // 使用数字1到7
    }
    
    std::cout << "生成的样本: ";
    for (int sample : samples) {
        std::cout << static_cast<char>('A' + sample - 1) << " ";  // 显示时转换为字母
    }
    std::cout << std::endl;
    
    std::cout << "\n=== 开始求解过程 ===" << std::endl;
    std::cout << "1. 初始化求解器..." << std::endl;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    std::cout << "\n2. 求解结果分析" << std::endl;
    std::cout << "  - 状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "  - 覆盖率: " << solution.coverageRatio * 100 << "%" << std::endl;
    std::cout << "  - 总组数: " << solution.totalGroups << std::endl;
    std::cout << "  - 计算时间: " << solution.computationTime << "秒" << std::endl;
    
    if (!solution.groups.empty()) {
        std::cout << "\n3. 选中的组分析" << std::endl;
        int groupCount = 0;
        for (const auto& group : solution.groups) {
            std::cout << "  组 " << ++groupCount << ": ";
            for (int elem : group) {
                std::cout << static_cast<char>(elem) << " ";
            }
            std::cout << std::endl;
        }
    }
    
    std::cout << "\n=== 开始详细覆盖率验证 ===" << std::endl;
    std::cout << "1. 验证解的状态..." << std::endl;
    if (solution.status != Status::Success) {
        std::cout << "❌ 解的状态不是Success，而是: " << static_cast<int>(solution.status) << std::endl;
        std::cout << "   状态码含义:" << std::endl;
        std::cout << "   - 0: Success (成功)" << std::endl;
        std::cout << "   - 1: Timeout (超时)" << std::endl;
        std::cout << "   - 2: NoSolution (无解)" << std::endl;
        std::cout << "   - 3: Error (错误)" << std::endl;
    } else {
        std::cout << "✅ 解的状态是Success" << std::endl;
    }
    
    std::cout << "\n2. 验证k组的有效性..." << std::endl;
    bool kGroupsValid = true;
    for (size_t i = 0; i < solution.groups.size(); ++i) {
        const auto& group = solution.groups[i];
        std::cout << "  检查组 " << (i + 1) << ": ";
        
        // 检查组大小
        if (group.size() != k) {
            std::cout << "❌ 组大小错误 (size=" << group.size() << ", 应为" << k << ")" << std::endl;
            kGroupsValid = false;
            continue;
        }
        
        // 检查元素有效性
        bool elementsValid = true;
        for (int elem : group) {
            if (std::find(samples.begin(), samples.end(), elem) == samples.end()) {
                std::cout << "❌ 无效元素: " << static_cast<char>(elem) << std::endl;
                elementsValid = false;
                kGroupsValid = false;
                break;
            }
        }
        
        if (elementsValid) {
            std::cout << "✅ 有效" << std::endl;
        }
    }
    
    std::cout << "\n3. 验证j组覆盖..." << std::endl;
    bool coverageValid = checkJGroupCoverage(solution, samples, j, s);
    std::cout << "覆盖验证结果: " << (coverageValid ? "✅ 通过" : "❌ 失败") << std::endl;
    
    if (!coverageValid) {
        std::cout << "覆盖失败分析:" << std::endl;
        std::cout << "  - 期望: 每个j组(" << j << "个元素)中至少有一个s子集(" << s << "个元素)被某个k组覆盖" << std::endl;
        std::cout << "  - 实际: 存在未被覆盖的j组" << std::endl;
    }
    
    std::cout << "\n4. 最终验证结果..." << std::endl;
    EXPECT_EQ(solution.status, Status::Success) << "解的状态应该是Success";
    EXPECT_TRUE(coverageValid) << "所有j组应该至少有一个s子集被覆盖";
    validateSolution(solution, samples, k);
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

TEST_F(ModeASetCoverSolverTest, Example4Test) {
    std::cout << "\n=== 开始 Example 4 测试 ===" << std::endl;
    
    int m = 45;
    int n = 8;
    int k = 6;
    int j = 6;
    int s = 5;
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", j=" << j << ", s=" << s << std::endl;
    
    // 使用固定的样本 1-8
    std::vector<int> samples;
    for (int i = 1; i <= n; i++) {
        samples.push_back(i);  // 使用数字1到8
    }
    
    // 标准答案也使用数字
    std::vector<std::vector<int>> standardSolution = {
        {1, 2, 3, 5, 6, 7},
        {1, 2, 4, 5, 6, 8},
        {1, 3, 4, 5, 7, 8},
        {2, 3, 4, 6, 7, 8}
    };
    
    std::cout << "输入样本: ";
    for (int sample : samples) {
        std::cout << numToLetter(sample) << " ";
    }
    std::cout << std::endl;
    
    std::cout << "开始求解..." << std::endl;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    std::cout << "求解完成。状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "覆盖率: " << solution.coverageRatio << std::endl;
    std::cout << "总组数: " << solution.totalGroups << " (标准答案: " << standardSolution.size() << ")" << std::endl;
    std::cout << "计算时间: " << solution.computationTime << "秒" << std::endl;
    
    std::cout << "\n生成的解:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }
    
    std::cout << "\n标准答案:" << std::endl;
    for (const auto& group : standardSolution) {
        std::cout << "  组: ";
        for (const auto& letter : group) {
            std::cout << numToLetter(letter);
        }
        std::cout << std::endl;
    }
    
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s));
    validateSolution(solution, samples, k);
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

TEST_F(ModeASetCoverSolverTest, Example6Test) {
    std::cout << "\n=== 开始 Example 6 测试 ===" << std::endl;
    
    int m = 45;
    int n = 9;
    int k = 6;
    int j = 5;
    int s = 4;
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", j=" << j << ", s=" << s << std::endl;
    
    // 使用固定的样本 1-9
    std::vector<int> samples;
    for (int i = 1; i <= n; i++) {
        samples.push_back(i);  // 使用数字1到9
    }
    
    // 标准答案也使用数字
    std::vector<std::vector<int>> standardSolution = {
        {1, 2, 4, 6, 8, 9},
        {1, 3, 5, 6, 7, 9},
        {2, 3, 4, 5, 7, 8}
    };
    
    std::cout << "\n输入样本: ";
    for (int sample : samples) {
        std::cout << numToLetter(sample) << " ";
    }
    std::cout << std::endl;
    
    // 生成组合并获取preprocessor结果
    auto combinations = generateTestCombinations(m, n, samples, k, s, j);
    auto preprocessResult = m_preprocessor->preprocess(
        samples,
        n,
        j,
        s,
        k,
        CoverageMode::CoverMinOneS,
        1,
        combinations.allSSubsets,
        combinations.jCombinations,
        combinations.sToJMap,
        combinations.jToSMap
    );
    
    std::cout << "开始求解..." << std::endl;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    std::cout << "求解完成。状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "覆盖率: " << solution.coverageRatio << std::endl;
    std::cout << "总组数: " << solution.totalGroups << " (标准答案: " << standardSolution.size() << ")" << std::endl;
    std::cout << "计算时间: " << solution.computationTime << "秒" << std::endl;
    
    std::cout << "\n=== Mode A 覆盖计算开始 ===" << std::endl;
    std::cout << "输入参数分析:" << std::endl;
    std::cout << "- k组数量: " << solution.totalGroups << std::endl;
    std::cout << "- j组数量: " << j << std::endl;
    std::cout << "- 每个j组的s子集数量: " << s << std::endl;
    std::cout << "- 最小覆盖要求: 1" << std::endl;
    std::cout << std::endl;

    // 打印preprocessor生成的候选s集合
    std::cout << "Preprocessor生成的候选s集合:" << std::endl;
    for (const auto& sSubset : preprocessResult.selectedSSubsets) {
        std::cout << "  子集: ";
        for (int elem : sSubset) {
            std::cout << numToLetter(elem) << " ";
        }
        auto it = preprocessResult.selectedSToJMap.find(sSubset);
        if (it != preprocessResult.selectedSToJMap.end()) {
            std::cout << "-> 覆盖 " << it->second.size() << " 个j组";
        }
        std::cout << std::endl;
    }
    std::cout << "总计 " << preprocessResult.selectedSSubsets.size() << " 个候选s子集" << std::endl;
    std::cout << std::endl;

    std::cout << "k组列表:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }

    std::cout << "\n线程配置:" << std::endl;
    std::cout << "- 可用CPU线程数: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "- 优化后使用线程数: " << m_config.threadCount << std::endl;
    std::cout << "- 每线程处理数据量: " << (j / m_config.threadCount) << std::endl;
    std::cout << std::endl;

    std::cout << "求解时间: " << solution.computationTime << " 秒" << std::endl;
    std::cout << std::endl;

    std::cout << "\n生成的解:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }
    
    std::cout << "\n标准答案:" << std::endl;
    for (const auto& group : standardSolution) {
        std::cout << "  组: ";
        for (int elem : group) {
            std::cout << numToLetter(elem);
        }
        std::cout << std::endl;
    }
    
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s));
    validateSolution(solution, samples, k);
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

TEST_F(ModeASetCoverSolverTest, Example7Test) {
    std::cout << "\n=== 开始 Example 7 测试 ===" << std::endl;
    
    int m = 45;
    int n = 10;
    int k = 6;
    int j = 6;
    int s = 4;
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", j=" << j << ", s=" << s << std::endl;
    
    // 使用固定的样本 1-10
    std::vector<int> samples;
    for (int i = 1; i <= n; i++) {
        samples.push_back(i);  // 使用数字1到10
    }
    
    // 标准答案也使用数字
    std::vector<std::vector<int>> standardSolution = {
        {1, 2, 5, 7, 9, 10},
        {1, 3, 5, 6, 8, 10},
        {2, 3, 4, 7, 8, 9}
    };
    
    std::cout << "输入样本: ";
    for (int sample : samples) {
        std::cout << numToLetter(sample) << " ";
    }
    std::cout << std::endl;
    
    std::cout << "开始求解..." << std::endl;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    std::cout << "求解完成。状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "覆盖率: " << solution.coverageRatio << std::endl;
    std::cout << "总组数: " << solution.totalGroups << " (标准答案: " << standardSolution.size() << ")" << std::endl;
    std::cout << "计算时间: " << solution.computationTime << "秒" << std::endl;
    
    std::cout << "\n生成的解:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }
    
    std::cout << "\n标准答案:" << std::endl;
    for (const auto& group : standardSolution) {
        std::cout << "  组: ";
        for (const auto& letter : group) {
            std::cout << numToLetter(letter);
        }
        std::cout << std::endl;
    }
    
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s));
    validateSolution(solution, samples, k);
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

TEST_F(ModeASetCoverSolverTest, Example8Test) {
    std::cout << "\n=== 开始 Example 8 测试 ===" << std::endl;
    
    // 测试参数
    int m = 45;  // universeSize
    int n = 12;  // 从m中选择的样本数量
    int k = 6;   // 每组样本数量
    int s = 3;   // 需要覆盖的子集大小
    int j = 6;   // j参数
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", j=" << j << ", s=" << s << std::endl;
    
    // 使用固定的样本 1-12
    std::vector<int> samples;
    for (int i = 1; i <= n; i++) {
        samples.push_back(i);  // 使用数字1到12
    }
    
    // 标准答案也使用数字
    std::vector<std::vector<int>> standardSolution = {
        {1, 2, 4, 7, 11, 12},   // ABDGKL
        {1, 3, 4, 8, 10, 12},   // ACDHJL
        {1, 4, 5, 6, 9, 12},    // ADEFIL
        {2, 3, 7, 8, 10, 11},   // BCGHJK
        {2, 5, 6, 7, 9, 11},    // BEFGIK
        {3, 5, 6, 8, 9, 10}     // CEFHIJ
    };
    
    std::cout << "输入样本: ";
    for (int sample : samples) {
        std::cout << numToLetter(sample) << " ";
    }
    std::cout << std::endl;
    
    // 生成所有j组合和s子集
    auto result = m_combGen->generateJCombinationsAndSSubsets(m, n, j, s);
    auto jCombinations = result.first;
    auto sSubsets = result.second;

    // 生成组合并获取preprocessor结果
    auto combinations = generateTestCombinations(m, n, samples, k, s, j);
    auto preprocessResult = m_preprocessor->preprocess(
        samples,
        n,
        j,
        s,
        k,
        CoverageMode::CoverMinOneS,
        1,
        combinations.allSSubsets,
        combinations.jCombinations,
        combinations.sToJMap,
        combinations.jToSMap
    );

    std::cout << "\n=== 验证标准答案的覆盖率 ===" << std::endl;
    std::cout << "j组数量: " << jCombinations.size() << std::endl;
    std::cout << "每个j组合的s子集数: " << (sSubsets.empty() ? 0 : sSubsets[0].size()) << std::endl;

    // 验证标准答案的覆盖率
    auto standardCoverage = m_covCalc->calculateCoverage(
        standardSolution,
        jCombinations,
        sSubsets,
        CoverageMode::CoverMinOneS,
        1  // 最小覆盖数为1
    );

    std::cout << "\n标准答案覆盖率分析：" << std::endl;
    std::cout << "- 总j组数量: " << standardCoverage.total_j_count << std::endl;
    std::cout << "- 已覆盖j组数量: " << standardCoverage.covered_j_count << std::endl;
    std::cout << "- 覆盖率: " << (standardCoverage.coverage_ratio * 100) << "%" << std::endl;
    
    // 验证标准答案的每个j组覆盖情况
    std::cout << "\n验证标准答案的j组覆盖..." << std::endl;
    DetailedSolution standardSol;
    standardSol.status = Status::Success;
    standardSol.groups = standardSolution;
    standardSol.coverageRatio = standardCoverage.coverage_ratio;
    standardSol.totalGroups = static_cast<int>(standardSolution.size());
    standardSol.computationTime = 0.0;
    standardSol.isOptimal = true;
    standardSol.message = "验证标准答案";

    bool standardCoverageValid = checkJGroupCoverage(
        standardSol,
        samples,
        j,
        s
    );
    
    EXPECT_TRUE(standardCoverageValid) << "标准答案未能完全覆盖所有j组";
    
    // 开始求解
    std::cout << "\n=== 开始求解 ===" << std::endl;
    auto solution = m_solver->solve(m, n, samples, k, s, j);
    
    // 输出结果
    std::cout << "求解完成。状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "覆盖率: " << solution.coverageRatio << std::endl;
    std::cout << "总组数: " << solution.totalGroups << " (标准答案: " << standardSolution.size() << ")" << std::endl;
    std::cout << "计算时间: " << solution.computationTime << "秒" << std::endl;
    
    // 验证生成的解
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_DOUBLE_EQ(solution.coverageRatio, standardCoverage.coverage_ratio);
    EXPECT_TRUE(solution.totalGroups >= static_cast<int>(standardSolution.size())) 
        << "生成的组数应该不少于标准答案的组数";
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s))
        << "生成的解应该满足覆盖要求";
    validateSolution(solution, samples, k);
    
    std::cout << "\n生成的解:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }
    
    std::cout << "\n标准答案:" << std::endl;
    for (const auto& group : standardSolution) {
        std::cout << "  组: ";
        for (const auto& letter : group) {
            std::cout << numToLetter(letter);
        }
        std::cout << std::endl;
    }
    
    std::cout << "=== Mode A 覆盖计算开始 ===" << std::endl;
    std::cout << "输入参数分析:" << std::endl;
    std::cout << "- k组数量: " << solution.totalGroups << std::endl;
    std::cout << "- j组数量: " << j << std::endl;
    std::cout << "- 每个j组的s子集数量: " << s << std::endl;
    std::cout << "- 最小覆盖要求: 1" << std::endl;
    std::cout << std::endl;

    // 打印preprocessor生成的候选s集合
    std::cout << "Preprocessor生成的候选s集合:" << std::endl;
    for (const auto& sSubset : preprocessResult.selectedSSubsets) {
        std::cout << "  子集: ";
        for (int elem : sSubset) {
            std::cout << numToLetter(elem) << " ";
        }
        auto it = preprocessResult.selectedSToJMap.find(sSubset);
        if (it != preprocessResult.selectedSToJMap.end()) {
            std::cout << "-> 覆盖 " << it->second.size() << " 个j组";
        }
        std::cout << std::endl;
    }
    std::cout << "总计 " << preprocessResult.selectedSSubsets.size() << " 个候选s子集" << std::endl;
    std::cout << std::endl;

    std::cout << "k组列表:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }

    std::cout << "\n线程配置:" << std::endl;
    std::cout << "- 可用CPU线程数: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "- 优化后使用线程数: " << m_config.threadCount << std::endl;
    std::cout << "- 每线程处理数据量: " << (j / m_config.threadCount) << std::endl;
    std::cout << std::endl;

    std::cout << "求解时间: " << solution.computationTime << " 秒" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

TEST_F(ModeASetCoverSolverTest, Example9Test) {
    std::cout << "\n=== 开始 Example 9 测试 ===" << std::endl;
    
    // 测试参数
    int m = 54;  // universeSize
    int n = 25;  // 从m中选择的样本数量
    int k = 7;   // 每组样本数量
    int s = 5;   // 需要覆盖的子集大小
    int j = 7;   // j参数
    
    std::cout << "测试参数: m=" << m << ", n=" << n << ", k=" << k << ", j=" << j << ", s=" << s << std::endl;
    
    // 使用固定的样本 1-25
    std::vector<int> samples;
    for (int i = 1; i <= n; i++) {
        samples.push_back(i);  // 使用数字1到25
    }
    
    std::cout << "输入样本: ";
    for (int sample : samples) {
        std::cout << numToLetter(sample) << " ";
    }
    std::cout << std::endl;
        
    // 生成所有j组合和s子集
    auto result = m_combGen->generateJCombinationsAndSSubsets(m, n, j, s);
    auto jCombinations = result.first;
    auto sSubsets = result.second;

    std::cout << "\n=== 开始求解 ===" << std::endl;
        auto solution = m_solver->solve(m, n, samples, k, s, j);
        
    // 输出结果
    std::cout << "求解完成。状态: " << (solution.status == Status::Success ? "成功" : "失败") << std::endl;
    std::cout << "覆盖率: " << solution.coverageRatio << std::endl;
            std::cout << "总组数: " << solution.totalGroups << std::endl;
            std::cout << "计算时间: " << solution.computationTime << "秒" << std::endl;
            
    // 验证生成的解
    EXPECT_EQ(solution.status, Status::Success);
    EXPECT_DOUBLE_EQ(solution.coverageRatio, 1.0);
    EXPECT_TRUE(checkJGroupCoverage(solution, samples, j, s))
        << "生成的解应该满足覆盖要求";
    validateSolution(solution, samples, k);

    std::cout << "\n生成的解:" << std::endl;
    for (const auto& group : solution.groups) {
        std::cout << "  组: " << groupToLetters(group) << std::endl;
    }
    
    std::cout << "=== 测试完成 ===" << std::endl;
}

} // namespace testing
} // namespace core_algo 

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 