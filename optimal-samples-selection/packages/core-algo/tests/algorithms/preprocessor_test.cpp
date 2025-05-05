#include <gtest/gtest.h>
#include "preprocessor.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <set>
#include <map>
#include <iomanip>  // 添加这个头文件用于 std::setprecision

namespace core_algo {
namespace {

class PreprocessorTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_combGen = CombinationGenerator::create();
        m_setOps = SetOperations::create();
        m_preprocessor = std::make_unique<Preprocessor>(m_combGen, m_setOps);
    }

    // 辅助函数：计算两个集合的Jaccard距离
    double calculateJaccardDistance(const std::vector<int>& set1, const std::vector<int>& set2) {
        std::set<int> intersection, union_set;
        for (int elem : set1) union_set.insert(elem);
        for (int elem : set2) {
            if (std::find(set1.begin(), set1.end(), elem) != set1.end()) {
                intersection.insert(elem);
            }
            union_set.insert(elem);
        }
        return 1.0 - static_cast<double>(intersection.size()) / union_set.size();
    }

    // 辅助函数：验证选择的s子集的多样性
    double validateDiversity(const std::vector<std::vector<int>>& selectedSSubsets) {
        if (selectedSSubsets.size() <= 1) return 1.0;
        
        double totalDistance = 0.0;
        int pairs = 0;
        for (size_t i = 0; i < selectedSSubsets.size(); i++) {
            for (size_t j = i + 1; j < selectedSSubsets.size(); j++) {
                totalDistance += calculateJaccardDistance(selectedSSubsets[i], selectedSSubsets[j]);
                pairs++;
            }
        }
        return pairs > 0 ? totalDistance / pairs : 1.0;
    }

    // 新增：验证s子集对是否能构建有效的k集合并输出具体的k集合
    bool validateKConstruction(
        const std::vector<std::vector<int>>& selectedSSubsets,
        const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
        const std::vector<std::vector<int>>& jGroups,
        int k
    ) {
        // 记录每个j组的覆盖情况
        struct JCoverage {
            std::vector<std::vector<int>> coveringSubsets;
            bool hasValidKSet = false;
            std::vector<int> kSet;
            std::pair<std::vector<int>, std::vector<int>> compatiblePair;
        };
        std::map<std::vector<int>, JCoverage> jCoverageMap;
        
        // 初始化j组的覆盖信息
        for (const auto& j : jGroups) {
            jCoverageMap[j] = JCoverage{};
        }
        
        // 收集每个j组被哪些s子集覆盖
        for (const auto& s : selectedSSubsets) {
            auto it = sToJMap.find(s);
            if (it != sToJMap.end()) {
                for (const auto& j : it->second) {
                    jCoverageMap[j].coveringSubsets.push_back(s);
                }
            }
        }
        
        // 对每个j组，检查是否能找到有效的k集合
        size_t invalidCount = 0;
        for (auto& [j, coverage] : jCoverageMap) {
            if (coverage.coveringSubsets.size() < 2) {
                invalidCount++;
                continue;
            }
            
            // 尝试所有可能的s子集对
            for (size_t i = 0; i < coverage.coveringSubsets.size(); ++i) {
                for (size_t m = i + 1; m < coverage.coveringSubsets.size(); ++m) {
                    const auto& s1 = coverage.coveringSubsets[i];
                    const auto& s2 = coverage.coveringSubsets[m];
                    
                    // 计算两个s子集的并集
                    std::set<int> kSet;
                    kSet.insert(s1.begin(), s1.end());
                    kSet.insert(s2.begin(), s2.end());
                    
                    // 检查并集大小是否不超过k
                    if (kSet.size() <= k) {
                        coverage.hasValidKSet = true;
                        coverage.kSet = std::vector<int>(kSet.begin(), kSet.end());
                        coverage.compatiblePair = std::make_pair(s1, s2);
                        break;
                    }
                }
                if (coverage.hasValidKSet) break;
            }
            
            if (!coverage.hasValidKSet) {
                invalidCount++;
            }
        }
        
        // 输出总体结果
        std::cout << "\n=== K集合构建验证结果 ===" << std::endl;
        std::cout << "总j组数量: " << jGroups.size() << std::endl;
        std::cout << "成功构建k集合的j组数量: " << (jGroups.size() - invalidCount) << std::endl;
        std::cout << "无法构建k集合的j组数量: " << invalidCount << std::endl;
        std::cout << "成功率: " << std::fixed << std::setprecision(2)
                  << ((jGroups.size() - invalidCount) * 100.0 / jGroups.size()) << "%" << std::endl;
        
        return invalidCount == 0;
    }

    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::unique_ptr<Preprocessor> m_preprocessor;
};

// 小参数测试：n=8, j=4, s=2
TEST_F(PreprocessorTest, SmallParameterTest) {
    std::vector<int> samples = {1, 2, 3, 4, 5, 6, 7, 8};
    int n = 8;
    int j = 4;
    int s = 2;
    
    auto result = m_preprocessor->preprocess(samples, n, j, s, 7, CoverageMode::CoverMinOneS);
    
    // 基本验证
    ASSERT_FALSE(result.allSSubsets.empty());
    ASSERT_FALSE(result.jGroups.empty());
    ASSERT_FALSE(result.selectedSSubsets.empty());
    
    // 验证组合数量
    size_t expectedSSubsets = m_combGen->generate(samples, s).size();
    size_t expectedJGroups = m_combGen->generate(samples, j).size();
    EXPECT_EQ(result.allSSubsets.size(), expectedSSubsets);
    EXPECT_EQ(result.jGroups.size(), expectedJGroups);
    
    // 验证映射关系
    for (const auto& jGroup : result.jGroups) {
        EXPECT_TRUE(result.jToSMap.find(jGroup) != result.jToSMap.end());
        auto sSubsetsInJ = result.jToSMap.at(jGroup);
        size_t expectedSInJ = m_combGen->generate(jGroup, s).size();
        EXPECT_EQ(sSubsetsInJ.size(), expectedSInJ);
    }
    
    // 验证覆盖情况
    std::set<std::vector<int>> coveredJGroups;
    for (const auto& selectedS : result.selectedSSubsets) {
        auto it = result.sToJMap.find(selectedS);
        ASSERT_TRUE(it != result.sToJMap.end());
        coveredJGroups.insert(it->second.begin(), it->second.end());
    }
    
    // 验证覆盖率
    double coverageRate = static_cast<double>(coveredJGroups.size()) / result.jGroups.size();
    EXPECT_GE(coverageRate, 0.9);
    
    // 验证多样性
    double diversity = validateDiversity(result.selectedSSubsets);
    EXPECT_GE(diversity, 0.3) << "选择的s子集多样性不足";
    
    // 验证选择数量的合理性
    size_t expectedClusters = std::min(
        static_cast<size_t>(std::ceil(std::sqrt(result.jGroups.size()))),
        result.allSSubsets.size()
    );
    EXPECT_LE(result.selectedSSubsets.size(), expectedClusters * 2)
        << "选择的s子集数量超出预期";
    
    // 输出详细信息
    std::cout << "\n=== 小参数测试结果 (k=7) ===" << std::endl;
    std::cout << "总s子集数量: " << result.allSSubsets.size() << std::endl;
    std::cout << "总j组数量: " << result.jGroups.size() << std::endl;
    std::cout << "选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
    std::cout << "覆盖的j组数量: " << coveredJGroups.size() << std::endl;
    std::cout << "覆盖率: " << (coverageRate * 100) << "%" << std::endl;
    std::cout << "多样性得分: " << diversity << std::endl;
    
    std::cout << "\n选择的s子集:" << std::endl;
    for (const auto& s : result.selectedSSubsets) {
        std::cout << "{ ";
        for (int elem : s) {
            std::cout << elem << " ";
        }
        std::cout << "}" << std::endl;
    }
}

// 中等规模参数测试
TEST_F(PreprocessorTest, MediumParameterTest) {
    std::vector<int> samples;
    for (int i = 1; i <= 15; ++i) {
        samples.push_back(i);
    }
    int n = 15;
    int j = 5;
    int s = 3;
    
    auto result = m_preprocessor->preprocess(samples, n, j, s, 7, CoverageMode::CoverMinNS);
    
    // 验证基本结果
    ASSERT_FALSE(result.selectedSSubsets.empty());
    
    // 验证覆盖情况
    std::set<std::vector<int>> coveredJGroups;
    for (const auto& selectedS : result.selectedSSubsets) {
        auto it = result.sToJMap.find(selectedS);
        ASSERT_TRUE(it != result.sToJMap.end());
        coveredJGroups.insert(it->second.begin(), it->second.end());
    }
    
    double coverageRate = static_cast<double>(coveredJGroups.size()) / result.jGroups.size();
    EXPECT_GE(coverageRate, 0.9);
    
    // 验证多样性
    double diversity = validateDiversity(result.selectedSSubsets);
    EXPECT_GE(diversity, 0.3);
    
    std::cout << "\n=== 中等规模测试结果 (k=7) ===" << std::endl;
    std::cout << "总s子集数量: " << result.allSSubsets.size() << std::endl;
    std::cout << "总j组数量: " << result.jGroups.size() << std::endl;
    std::cout << "选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
    std::cout << "覆盖率: " << (coverageRate * 100) << "%" << std::endl;
    std::cout << "多样性得分: " << diversity << std::endl;
}

// 大规模参数测试
TEST_F(PreprocessorTest, LargeParameterTest) {
    std::vector<int> samples;
    for (int i = 1; i <= 25; ++i) {
        samples.push_back(i);
    }
    
    auto result = m_preprocessor->preprocess(samples, 25, 7, 3, 7, CoverageMode::CoverMinOneS);
    
    // 验证选择的s子集数量
    size_t expectedClusters = std::min(
        static_cast<size_t>(std::ceil(std::sqrt(result.jGroups.size()))),
        result.allSSubsets.size()
    );
    EXPECT_LE(result.selectedSSubsets.size(), expectedClusters * 2);
    
    // 验证覆盖情况
    std::set<std::vector<int>> coveredJGroups;
    for (const auto& selectedS : result.selectedSSubsets) {
        auto it = result.sToJMap.find(selectedS);
        ASSERT_TRUE(it != result.sToJMap.end());
        coveredJGroups.insert(it->second.begin(), it->second.end());
    }
    
    double coverageRate = static_cast<double>(coveredJGroups.size()) / result.jGroups.size();
    EXPECT_GE(coverageRate, 0.9);
    
    // 验证多样性
    double diversity = validateDiversity(result.selectedSSubsets);
    EXPECT_GE(diversity, 0.3);
    
    std::cout << "\n=== 大规模测试结果 (k=7) ===" << std::endl;
    std::cout << "总j组数量: " << result.jGroups.size() << std::endl;
    std::cout << "选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
    std::cout << "覆盖率: " << (coverageRate * 100) << "%" << std::endl;
    std::cout << "多样性得分: " << diversity << std::endl;
}

// Mode B策略测试：n=8, j=6, s=5
TEST_F(PreprocessorTest, ModeBStrategyTest) {
    std::vector<int> samples = {1, 2, 3, 4, 5, 6, 7, 8};
    int n = 8;
    int j = 6;
    int s = 5;
    int k = 7;  // 明确指定k值
    int minCoverageCount = 2;
    
    std::cout << "\n=== Mode B 策略测试开始 ===" << std::endl;
    std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k << std::endl;
    std::cout << "最小覆盖次数要求: " << minCoverageCount << std::endl;
    
    auto result = m_preprocessor->preprocess(samples, n, j, s, k, CoverageMode::CoverMinNS);
    
    // 基本验证
    ASSERT_FALSE(result.allSSubsets.empty());
    ASSERT_FALSE(result.jGroups.empty());
    ASSERT_FALSE(result.selectedSSubsets.empty());
    
    // 验证组合数量
    size_t expectedSSubsets = m_combGen->generate(samples, s).size();
    size_t expectedJGroups = m_combGen->generate(samples, j).size();
    EXPECT_EQ(result.allSSubsets.size(), expectedSSubsets);
    EXPECT_EQ(result.jGroups.size(), expectedJGroups);
    
    // 验证每个j组的覆盖次数
    std::map<std::vector<int>, int> jCoverageCount;
    for (const auto& jGroup : result.jGroups) {
        jCoverageCount[jGroup] = 0;
    }
    
    for (const auto& selectedS : result.selectedSSubsets) {
        auto it = result.sToJMap.find(selectedS);
        ASSERT_TRUE(it != result.sToJMap.end());
        for (const auto& jGroup : it->second) {
            jCoverageCount[jGroup]++;
        }
    }
    
    // 验证每个j组至少被覆盖N次
    int insufficientCoverage = 0;
    for (const auto& [jGroup, count] : jCoverageCount) {
        if (count < minCoverageCount) {
            insufficientCoverage++;
            std::cout << "警告: j组 {";
            for (int elem : jGroup) {
                std::cout << elem << " ";
            }
            std::cout << "} 只被覆盖了 " << count << " 次" << std::endl;
        }
    }
    
    // 验证覆盖质量
    double coverageQuality = 1.0 - static_cast<double>(insufficientCoverage) / result.jGroups.size();
    EXPECT_GE(coverageQuality, 0.9) << "有超过10%的j组覆盖次数不足" << minCoverageCount << "次";
    
    // 验证多样性
    double diversity = validateDiversity(result.selectedSSubsets);
    EXPECT_GE(diversity, 0.3) << "选择的s子集多样性不足";
    
    // 添加k集合构建验证
    bool kConstructionValid = validateKConstruction(
        result.selectedSSubsets,
        result.sToJMap,
        result.jGroups,
        k
    );
    EXPECT_TRUE(kConstructionValid) << "存在无法构建有效k集合的j组";
    
    // 输出详细信息
    std::cout << "\n=== Mode B 测试结果 ===" << std::endl;
    std::cout << "总s子集数量: " << result.allSSubsets.size() << std::endl;
    std::cout << "总j组数量: " << result.jGroups.size() << std::endl;
    std::cout << "选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
    std::cout << "覆盖质量: " << (coverageQuality * 100) << "%" << std::endl;
    std::cout << "多样性得分: " << diversity << std::endl;
    
    // 输出选择的s子集
    std::cout << "\n选择的s子集:" << std::endl;
    for (const auto& s : result.selectedSSubsets) {
        std::cout << "{ ";
        for (int elem : s) {
            std::cout << elem << " ";
        }
        std::cout << "}" << std::endl;
    }
    
    // 输出覆盖统计
    std::cout << "\n覆盖统计:" << std::endl;
    std::map<int, int> coverageDistribution;
    for (const auto& [_, count] : jCoverageCount) {
        coverageDistribution[count]++;
    }
    for (const auto& [count, frequency] : coverageDistribution) {
        std::cout << count << " 次覆盖: " << frequency << " 个j组" << std::endl;
    }
    
    std::cout << "=== Mode B 策略测试结束 ===\n" << std::endl;
}

// Mode B 极限参数测试
TEST_F(PreprocessorTest, ModeBEdgeCaseTest) {
    std::cout << "\n=== Mode B 极限参数测试开始 ===" << std::endl;
    
    // 测试用例1：最小参数 - 元素集中分布
    {
        std::vector<int> samples;
        for (int i = 1; i <= 7; i++) {
            samples.push_back(i);
        }
        int n = 7, j = 4, s = 3, k = 4;
        
        std::cout << "\n测试用例1: 最小参数（元素集中分布）" << std::endl;
        std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k << std::endl;
        
        auto result = m_preprocessor->preprocess(samples, n, j, s, k, CoverageMode::CoverMinNS);
        
        // 验证并输出结果
        double diversity = validateDiversity(result.selectedSSubsets);
        bool kConstructionValid = validateKConstruction(
            result.selectedSSubsets,
            result.sToJMap,
            result.jGroups,
            k
        );
        
        std::cout << "结果: " << std::endl;
        std::cout << "- 选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
        std::cout << "- 多样性得分: " << std::fixed << std::setprecision(2) << (diversity * 100) << "%" << std::endl;
        std::cout << "- k集合构建: " << (kConstructionValid ? "成功" : "失败") << std::endl;
        
        // 验证
        ASSERT_FALSE(result.selectedSSubsets.empty());
        EXPECT_LE(result.selectedSSubsets.size(), static_cast<size_t>(k * 1.5));
        EXPECT_GE(diversity, 0.7);
        EXPECT_TRUE(kConstructionValid);
    }
    
    // 测试用例2：最大参数 - 元素分散分布
    {
        std::vector<int> samples;
        for (int i = 1; i <= 25; i++) {
            samples.push_back(i);
        }
        int n = 25, j = 7, s = 5, k = 7;
        
        std::cout << "\n测试用例2: 最大参数（元素分散分布）" << std::endl;
        std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k << std::endl;
        
        auto result = m_preprocessor->preprocess(samples, n, j, s, k, CoverageMode::CoverMinNS);
        
        // 验证并输出结果
        double diversity = validateDiversity(result.selectedSSubsets);
        bool kConstructionValid = validateKConstruction(
            result.selectedSSubsets,
            result.sToJMap,
            result.jGroups,
            k
        );
        
        std::cout << "结果: " << std::endl;
        std::cout << "- 选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
        std::cout << "- 多样性得分: " << std::fixed << std::setprecision(2) << (diversity * 100) << "%" << std::endl;
        std::cout << "- k集合构建: " << (kConstructionValid ? "成功" : "失败") << std::endl;
        
        // 验证
        ASSERT_FALSE(result.selectedSSubsets.empty());
        EXPECT_LE(result.selectedSSubsets.size(), static_cast<size_t>(k * 1.5));
        EXPECT_GE(diversity, 0.7);
        EXPECT_TRUE(kConstructionValid);
    }
    
    // 测试用例3：特殊边界情况 (s接近k)
    {
        std::vector<int> samples;
        for (int i = 1; i <= 15; i++) {
            samples.push_back(i);
        }
        int n = 15, j = 6, s = 5, k = 6;
        
        std::cout << "\n测试用例3: 特殊边界情况 (s接近k)" << std::endl;
        std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k << std::endl;
        
        auto result = m_preprocessor->preprocess(samples, n, j, s, k, CoverageMode::CoverMinNS);
        
        // 验证并输出结果
        double diversity = validateDiversity(result.selectedSSubsets);
        bool kConstructionValid = validateKConstruction(
            result.selectedSSubsets,
            result.sToJMap,
            result.jGroups,
            k
        );
        
        std::cout << "结果: " << std::endl;
        std::cout << "- 选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
        std::cout << "- 多样性得分: " << std::fixed << std::setprecision(2) << (diversity * 100) << "%" << std::endl;
        std::cout << "- k集合构建: " << (kConstructionValid ? "成功" : "失败") << std::endl;
        
        // 验证
        ASSERT_FALSE(result.selectedSSubsets.empty());
        EXPECT_LE(result.selectedSSubsets.size(), static_cast<size_t>(k * 1.5));
        EXPECT_GE(diversity, 0.7);
        EXPECT_TRUE(kConstructionValid);
    }
    
    // 测试用例4：极端间隔情况
    {
        std::vector<int> samples;
        for (int i = 1; i <= 20; i++) {
            samples.push_back(i);
        }
        int n = 20, j = 6, s = 3, k = 7;
        
        std::cout << "\n测试用例4: 极端间隔情况" << std::endl;
        std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k << std::endl;
        
        auto result = m_preprocessor->preprocess(samples, n, j, s, k, CoverageMode::CoverMinNS);
        
        // 验证并输出结果
        double diversity = validateDiversity(result.selectedSSubsets);
        bool kConstructionValid = validateKConstruction(
            result.selectedSSubsets,
            result.sToJMap,
            result.jGroups,
            k
        );
        
        // 计算间隔比率
        auto validateGaps = [](const std::vector<std::vector<int>>& subsets) {
            double totalGapRatio = 0.0;
            for (const auto& s : subsets) {
                std::vector<int> sorted_s = s;
                std::sort(sorted_s.begin(), sorted_s.end());
                int gaps = 0;
                for (size_t i = 1; i < sorted_s.size(); ++i) {
                    gaps += sorted_s[i] - sorted_s[i-1] - 1;
                }
                totalGapRatio += static_cast<double>(gaps) / (sorted_s.back() - sorted_s.front());
            }
            return totalGapRatio / subsets.size();
        };
        double avgGapRatio = validateGaps(result.selectedSSubsets);
        
        std::cout << "结果: " << std::endl;
        std::cout << "- 选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
        std::cout << "- 多样性得分: " << std::fixed << std::setprecision(2) << (diversity * 100) << "%" << std::endl;
        std::cout << "- 平均间隔比率: " << std::fixed << std::setprecision(2) << (avgGapRatio * 100) << "%" << std::endl;
        std::cout << "- k集合构建: " << (kConstructionValid ? "成功" : "失败") << std::endl;
        
        // 验证
        ASSERT_FALSE(result.selectedSSubsets.empty());
        EXPECT_LE(result.selectedSSubsets.size(), static_cast<size_t>(k * 1.5));
        EXPECT_GE(diversity, 0.7);
        EXPECT_GE(avgGapRatio, 0.2);
        EXPECT_LE(avgGapRatio, 0.8);
        EXPECT_TRUE(kConstructionValid);
    }
    
    std::cout << "\n=== Mode B 极限参数测试结束 ===\n" << std::endl;
}

} // namespace
} // namespace core_algo

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 