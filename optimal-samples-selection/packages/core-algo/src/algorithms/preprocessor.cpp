#include "preprocessor.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <set>
#include <iostream>
#include <iomanip>

namespace core_algo {

Preprocessor::Preprocessor(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps
) : m_combGen(combGen), m_setOps(setOps) {}

std::unique_ptr<Preprocessor::SelectionStrategy> Preprocessor::createStrategy(CoverageMode mode) const {
    switch (mode) {
        case CoverageMode::CoverMinOneS:
            return std::make_unique<ModeAStrategy>();
        case CoverageMode::CoverMinNS:
            return std::make_unique<ModeBStrategy>();
        case CoverageMode::CoverAllS:
            return std::make_unique<ModeCStrategy>();
        default:
            throw std::invalid_argument("未知的覆盖模式");
    }
}

PreprocessResult Preprocessor::preprocess(
    const std::vector<int>& samples,
    int n,
    int j,
    int s,
    int k,
    CoverageMode mode,
    int minCoverageCount,
    const std::vector<std::vector<int>>& existingAllSSubsets,
    const std::vector<std::vector<int>>& existingJGroups,
    const std::map<std::vector<int>, std::vector<std::vector<int>>>& existingSToJMap,
    const std::map<std::vector<int>, std::vector<std::vector<int>>>& existingJToSMap
) {
    std::cout << "\n=== Preprocessor::preprocess 开始 ===" << std::endl;
    std::cout << "参数: n=" << n << ", j=" << j << ", s=" << s << ", k=" << k;
    if (mode == CoverageMode::CoverMinNS) {
        std::cout << ", minCoverageCount=" << minCoverageCount;
    }
    std::cout << std::endl;
    std::cout << "输入样本: ";
    for (int sample : samples) {
        std::cout << sample << " ";
    }
    std::cout << std::endl;
    
    PreprocessResult result;
    
    // 1. 使用已有数据或生成新的数据
    if (!existingAllSSubsets.empty()) {
        result.allSSubsets = existingAllSSubsets;
    } else {
        result.allSSubsets = m_combGen->generate(samples, s);
    }
    std::cout << "s子集数量: " << result.allSSubsets.size() << std::endl;
    
    if (!existingJGroups.empty()) {
        result.jGroups = existingJGroups;
    } else {
        result.jGroups = m_combGen->generate(samples, j);
    }
    std::cout << "j组数量: " << result.jGroups.size() << std::endl;
    
    // 2. 使用已有映射或建立新的映射
    if (!existingSToJMap.empty() && !existingJToSMap.empty()) {
        result.sToJMap = existingSToJMap;
        result.jToSMap = existingJToSMap;
    } else {
        size_t totalSSubsetsInJ = 0;
        for (const auto& jGroup : result.jGroups) {
            auto sSubsetsForJ = m_combGen->generate(jGroup, s);
            totalSSubsetsInJ += sSubsetsForJ.size();
            result.jToSMap[jGroup] = sSubsetsForJ;
            
            for (const auto& sSubset : sSubsetsForJ) {
                result.sToJMap[sSubset].push_back(jGroup);
            }
        }
        std::cout << "j组中s子集的平均数量: " << std::fixed << std::setprecision(2) 
                  << static_cast<double>(totalSSubsetsInJ) / result.jGroups.size() << std::endl;
    }
    
    // 3. 根据mode选择策略并执行selectTopS
    auto strategy = createStrategy(mode);
    result.selectedSSubsets = strategy->selectTopS(
        result.allSSubsets,
        result.sToJMap,
        result.jGroups,
        n,
        k
    );
    
    std::cout << "选择的s子集数量: " << result.selectedSSubsets.size() << std::endl;
    std::cout << "=== Preprocessor::preprocess 结束 ===\n" << std::endl;
    
    return result;
}

// 计算理论上需要多少个s子集才能覆盖所有j组
size_t calculateTheoreticalTopSCount(int n, int j, int s) {
    // 计算所有可能的j组数量
    double totalJ = 1.0;
    for (int i = 0; i < j; i++) {
        totalJ *= (n - i);
        totalJ /= (i + 1);
    }
    
    // 计算每个s能覆盖多少个j组
    double sCoversJ = 1.0;
    for (int i = 0; i < j-s; i++) {
        sCoversJ *= (n - s - i);
        sCoversJ /= (i + 1);
    }
    
    // 理论上需要的s数量 = ⌈总j组数 / 每个s覆盖的j组数⌉
    return static_cast<size_t>(std::ceil(totalJ / sCoversJ));
}

// Mode A: 每个j组至少有一个s子集被选中
std::vector<std::vector<int>> Preprocessor::ModeAStrategy::selectTopS(
    const std::vector<std::vector<int>>& allSSubsets,
    const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
    const std::vector<std::vector<int>>& jGroups,
    int n,
    int k
) {
    std::cout << "\n=== ModeA::selectTopS 开始 ===" << std::endl;
    
    std::vector<std::vector<int>> selectedS;
    std::set<std::vector<int>> coveredJGroups;
    
    // 第一阶段：大步覆盖
    // 设置较高的覆盖阈值，只选择能显著增加覆盖的s子集
    const double PHASE1_COVERAGE_THRESHOLD = 0.05;  // 要求每个s至少覆盖5%的未覆盖j组
    std::cout << "阶段1 - 大步覆盖（覆盖阈值: " << (PHASE1_COVERAGE_THRESHOLD * 100) << "%）" << std::endl;
    
    bool continuePhase1 = true;
    while (continuePhase1 && coveredJGroups.size() < jGroups.size()) {
        size_t maxNewCoverage = 0;
        std::vector<int> bestSubset;
        double bestJaccardMin = 0.0;  // 与已选集合的最小Jaccard距离
        
        for (const auto& subset : allSSubsets) {
            if (std::find(selectedS.begin(), selectedS.end(), subset) != selectedS.end()) {
                continue;
            }
            
            auto it = sToJMap.find(subset);
            if (it == sToJMap.end()) continue;
            
            // 计算新增覆盖
            size_t newCoverage = 0;
            for (const auto& jGroup : it->second) {
                if (coveredJGroups.find(jGroup) == coveredJGroups.end()) {
                    newCoverage++;
                }
            }
            
            // 计算覆盖率增量
            double coverageIncrease = static_cast<double>(newCoverage) / jGroups.size();
            
            // 如果覆盖增量达到阈值
            if (coverageIncrease >= PHASE1_COVERAGE_THRESHOLD) {
                // 计算与已选集合的最小Jaccard距离
                double minJaccard = 1.0;
                for (const auto& selected : selectedS) {
                    std::set<int> intersection, union_set;
                    for (int elem : subset) union_set.insert(elem);
                    for (int elem : selected) {
                        if (std::find(subset.begin(), subset.end(), elem) != subset.end()) {
                            intersection.insert(elem);
                        }
                        union_set.insert(elem);
                    }
                    double jaccard = 1.0 - static_cast<double>(intersection.size()) / union_set.size();
                    minJaccard = std::min(minJaccard, jaccard);
                }
                
                // 在覆盖率相同的情况下优先选择多样性更高的
                if (newCoverage > maxNewCoverage || 
                    (newCoverage == maxNewCoverage && minJaccard > bestJaccardMin)) {
                    maxNewCoverage = newCoverage;
                    bestSubset = subset;
                    bestJaccardMin = minJaccard;
                }
            }
        }
        
        if (maxNewCoverage > 0) {
            selectedS.push_back(bestSubset);
            auto it = sToJMap.find(bestSubset);
            for (const auto& jGroup : it->second) {
                coveredJGroups.insert(jGroup);
            }
            
            std::cout << "选择 s: {";
            for (size_t idx = 0; idx < bestSubset.size(); ++idx) {
                if (idx > 0) std::cout << ",";
                std::cout << bestSubset[idx];
            }
            std::cout << "}, 新增覆盖: " << maxNewCoverage
                      << ", 当前总覆盖: " << coveredJGroups.size() << "/" << jGroups.size()
                      << " (" << std::fixed << std::setprecision(2)
                      << (static_cast<double>(coveredJGroups.size()) / jGroups.size() * 100)
                      << "%)" << std::endl;
        } else {
            continuePhase1 = false;
        }
    }
    
    // 第二阶段：低重叠精调
    // 使用更低的覆盖阈值，但要求与已选集合有较高的Jaccard距离
    const double PHASE2_COVERAGE_THRESHOLD = 0.001;  // 要求至少覆盖0.1%的未覆盖j组
    const double PHASE2_JACCARD_THRESHOLD = 0.5;    // 要求与已选集合的Jaccard距离至少为0.5
    
    std::cout << "\n阶段2 - 低重叠精调（覆盖阈值: " << (PHASE2_COVERAGE_THRESHOLD * 100) 
              << "%, Jaccard阈值: " << (PHASE2_JACCARD_THRESHOLD * 100) << "%）" << std::endl;
    
    while (coveredJGroups.size() < jGroups.size()) {
        size_t maxNewCoverage = 0;
        std::vector<int> bestSubset;
        double bestJaccardMin = 0.0;
        
        for (const auto& subset : allSSubsets) {
            if (std::find(selectedS.begin(), selectedS.end(), subset) != selectedS.end()) {
                continue;
            }
            
            auto it = sToJMap.find(subset);
            if (it == sToJMap.end()) continue;
            
            // 计算与已选集合的最小Jaccard距离
            double minJaccard = 1.0;
            for (const auto& selected : selectedS) {
                std::set<int> intersection, union_set;
                for (int elem : subset) union_set.insert(elem);
                for (int elem : selected) {
                    if (std::find(subset.begin(), subset.end(), elem) != subset.end()) {
                        intersection.insert(elem);
                    }
                    union_set.insert(elem);
                }
                double jaccard = 1.0 - static_cast<double>(intersection.size()) / union_set.size();
                minJaccard = std::min(minJaccard, jaccard);
            }
            
            // 只考虑与已选集合有足够距离的子集
            if (minJaccard >= PHASE2_JACCARD_THRESHOLD) {
                size_t newCoverage = 0;
                for (const auto& jGroup : it->second) {
                    if (coveredJGroups.find(jGroup) == coveredJGroups.end()) {
                        newCoverage++;
                    }
                }
                
                double coverageIncrease = static_cast<double>(newCoverage) / jGroups.size();
                if (coverageIncrease >= PHASE2_COVERAGE_THRESHOLD && 
                    (newCoverage > maxNewCoverage || 
                     (newCoverage == maxNewCoverage && minJaccard > bestJaccardMin))) {
                    maxNewCoverage = newCoverage;
                    bestSubset = subset;
                    bestJaccardMin = minJaccard;
                }
            }
        }
        
        if (maxNewCoverage > 0) {
            selectedS.push_back(bestSubset);
            auto it = sToJMap.find(bestSubset);
            for (const auto& jGroup : it->second) {
                coveredJGroups.insert(jGroup);
            }
            
            std::cout << "选择 s: {";
            for (size_t idx = 0; idx < bestSubset.size(); ++idx) {
                if (idx > 0) std::cout << ",";
                std::cout << bestSubset[idx];
            }
            std::cout << "}, 新增覆盖: " << maxNewCoverage
                      << ", Jaccard距离: " << std::fixed << std::setprecision(2) 
                      << (bestJaccardMin * 100) << "%"
                      << ", 当前总覆盖: " << coveredJGroups.size() << "/" << jGroups.size()
                      << " (" << std::fixed << std::setprecision(2)
                      << (static_cast<double>(coveredJGroups.size()) / jGroups.size() * 100)
                      << "%)" << std::endl;
        } else {
            break;
        }
    }
    
    // 第三阶段：小步补齐
    // 不设置任何阈值限制，只要能增加覆盖就选择
    std::cout << "\n阶段3 - 小步补齐" << std::endl;
    
    bool hasImprovement;
    do {
        hasImprovement = false;
        size_t maxNewCoverage = 0;
        std::vector<int> bestSubset;
        
        for (const auto& subset : allSSubsets) {
            if (std::find(selectedS.begin(), selectedS.end(), subset) != selectedS.end()) {
                continue;
            }
            
            auto it = sToJMap.find(subset);
            if (it == sToJMap.end()) continue;
            
            size_t newCoverage = 0;
            for (const auto& jGroup : it->second) {
                if (coveredJGroups.find(jGroup) == coveredJGroups.end()) {
                    newCoverage++;
                }
            }
            
            if (newCoverage > maxNewCoverage) {
                maxNewCoverage = newCoverage;
                bestSubset = subset;
            }
        }
        
        if (maxNewCoverage > 0) {
            hasImprovement = true;
            selectedS.push_back(bestSubset);
            auto it = sToJMap.find(bestSubset);
            for (const auto& jGroup : it->second) {
                coveredJGroups.insert(jGroup);
            }
            
            std::cout << "选择 s: {";
            for (size_t idx = 0; idx < bestSubset.size(); ++idx) {
                if (idx > 0) std::cout << ",";
                std::cout << bestSubset[idx];
            }
            std::cout << "}, 新增覆盖: " << maxNewCoverage
                      << ", 当前总覆盖: " << coveredJGroups.size() << "/" << jGroups.size()
                      << " (" << std::fixed << std::setprecision(2)
                      << (static_cast<double>(coveredJGroups.size()) / jGroups.size() * 100)
                      << "%)" << std::endl;
        }
    } while (hasImprovement && coveredJGroups.size() < jGroups.size());
    
    std::cout << "最终选择了 " << selectedS.size() << " 个s子集" << std::endl;
    std::cout << "最终覆盖了 " << coveredJGroups.size() << "/" << jGroups.size() 
              << " 个j组 (" << std::fixed << std::setprecision(2)
              << (static_cast<double>(coveredJGroups.size()) / jGroups.size() * 100)
              << "%)" << std::endl;
    std::cout << "=== ModeA::selectTopS 结束 ===\n" << std::endl;
    
    return selectedS;
}

// Mode B: 每个j组至少有N个不同的s子集被选中
std::vector<std::vector<int>> Preprocessor::ModeBStrategy::selectTopS(
    const std::vector<std::vector<int>>& allSSubsets,
    const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
    const std::vector<std::vector<int>>& jGroups,
    int n,
    int k
) {
    std::cout << "\n=== ModeB::selectTopS 开始 ===" << std::endl;
    std::cout << "参数: k=" << k << std::endl;
    
    std::vector<std::vector<int>> selectedS;
    
    // 维护每个j组的覆盖次数
    std::map<std::vector<int>, int> jCoverageCount;
    for (const auto& j : jGroups) {
        jCoverageCount[j] = 0;
    }
    
    // 计算每个s子集的得分
    struct SubsetScore {
        std::vector<int> subset;
        double coverage_score;     // 覆盖得分
        double diversity_score;    // 多样性得分
        double total_score;        // 总得分
    };
    
    auto calculateScore = [&](const std::vector<int>& s) -> SubsetScore {
        SubsetScore score;
        score.subset = s;
        
        // 1. 计算覆盖得分：根据公式 score(s) = ∑max(0, 2-coverage[j])
        double coverage_score = 0.0;
        auto it = sToJMap.find(s);
        if (it != sToJMap.end()) {
            for (const auto& j : it->second) {
                coverage_score += std::max(0.0, 2.0 - jCoverageCount[j]);
            }
        }
        
        // 2. 计算多样性得分：与已选s的Jaccard距离
        double min_jaccard = 1.0;
        for (const auto& selected : selectedS) {
            std::set<int> intersection, union_set;
            for (int elem : s) union_set.insert(elem);
            for (int elem : selected) {
                if (std::find(s.begin(), s.end(), elem) != s.end()) {
                    intersection.insert(elem);
                }
                union_set.insert(elem);
            }
            double jaccard_distance = 1.0 - static_cast<double>(intersection.size()) / union_set.size();
            min_jaccard = std::min(min_jaccard, jaccard_distance);
        }
        
        score.coverage_score = coverage_score;
        score.diversity_score = min_jaccard;
        score.total_score = coverage_score * min_jaccard;  // 综合得分
        
        return score;
    };
    
    // 持续选择s直到所有j组都被覆盖至少2次
    bool all_covered_twice = false;
    size_t iteration = 0;
    const size_t MAX_ITERATIONS = jGroups.size() * 2;  // 设置最大迭代次数防止无限循环
    
    while (!all_covered_twice && iteration < MAX_ITERATIONS) {
        // 计算所有候选s的得分
        std::vector<SubsetScore> candidates;
        for (const auto& s : allSSubsets) {
            candidates.push_back(calculateScore(s));
        }
        
        // 选择得分最高的s
        auto best_candidate = std::max_element(
            candidates.begin(),
            candidates.end(),
            [](const SubsetScore& a, const SubsetScore& b) {
                return a.total_score < b.total_score;
            }
        );
        
        if (best_candidate == candidates.end() || best_candidate->total_score <= 0) {
            break;  // 没有有效的候选s了
        }
        
        // 添加选中的s
        selectedS.push_back(best_candidate->subset);
        
        // 更新覆盖计数
        auto it = sToJMap.find(best_candidate->subset);
        if (it != sToJMap.end()) {
            for (const auto& j : it->second) {
                jCoverageCount[j]++;
            }
        }
        
        // 输出选择信息
        std::cout << "选择 s: {";
        for (size_t i = 0; i < best_candidate->subset.size(); ++i) {
            if (i > 0) std::cout << ",";
            std::cout << best_candidate->subset[i];
        }
        std::cout << "}, 覆盖得分: " << std::fixed << std::setprecision(2) 
                 << best_candidate->coverage_score << ", 多样性得分: "
                 << (best_candidate->diversity_score * 100) << "%" << std::endl;
        
        // 检查是否所有j组都被覆盖至少2次
        all_covered_twice = true;
        for (const auto& [j, count] : jCoverageCount) {
            if (count < 2) {
                all_covered_twice = false;
                break;
            }
        }
        
        iteration++;
    }
    
    // 输出覆盖统计
    std::cout << "\n覆盖统计:" << std::endl;
    std::map<int, int> coverage_distribution;
    for (const auto& [_, count] : jCoverageCount) {
        coverage_distribution[count]++;
    }
    for (const auto& [count, frequency] : coverage_distribution) {
        std::cout << count << "次覆盖: " << frequency << " 个j组" << std::endl;
    }
    
    std::cout << "\n最终选择了 " << selectedS.size() << " 个s子集" << std::endl;
    std::cout << "=== ModeB::selectTopS 结束 ===\n" << std::endl;
    
    return selectedS;
}

// Mode C: 每个j组的所有s子集都被选中
std::vector<std::vector<int>> Preprocessor::ModeCStrategy::selectTopS(
    const std::vector<std::vector<int>>& allSSubsets,
    const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
    const std::vector<std::vector<int>>& jGroups,
    int n,
    int k
) {
    std::vector<std::vector<int>> selectedS;
    return selectedS;
}

} // namespace core_algo 