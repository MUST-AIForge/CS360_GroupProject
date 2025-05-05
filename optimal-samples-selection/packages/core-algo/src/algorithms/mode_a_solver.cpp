#include "mode_a_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include "preprocessor.hpp"
#include <algorithm>
#include <chrono>
#include <map>
#include <set>
#include <iostream>
#include <string>

namespace core_algo {

class ModeASetCoverSolverImpl : public ModeASolver {
private:
    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
    std::shared_ptr<CoverageCalculator> m_covCalc;
    std::shared_ptr<Preprocessor> m_preprocessor;

public:
    ModeASetCoverSolverImpl(
        std::shared_ptr<CombinationGenerator> combGen,
        std::shared_ptr<SetOperations> setOps,
        std::shared_ptr<CoverageCalculator> covCalc,
        const Config& config
    ) : ModeASolver(config) {
        m_combGen = combGen;
        m_setOps = setOps;
        m_covCalc = covCalc;
        m_preprocessor = std::make_shared<Preprocessor>(combGen, setOps);
    }

protected:
    CombinationResult generateCombinations(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j
    ) override {
        CombinationResult result;
        
        // 生成所有组合
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
        
        // 保存到求解器成员变量中
        jGroups_ = result.jCombinations;
        candidates_ = result.groups;
        allSSubsets_ = result.allSSubsets;
        jToSMap_ = result.jToSMap;
        sToJMap_ = result.sToJMap;
        
        return result;
    }

    std::vector<std::vector<int>> performSelection(
        const std::vector<std::vector<int>>& originalGroups,
        const std::vector<std::vector<int>>& jCombinations,
        const std::vector<std::vector<int>>& sSubsets,
        int j,
        int s
    ) const override {
        const int BEAM_WIDTH = 40;  // 增加beam宽度
        const int MAX_ITERATIONS = 200;  // 增加最大迭代次数
        const double MIN_GAIN = 0.05;  // 最小覆盖增益阈值
        
        std::cout << "\n=== performSelection开始 ===" << std::endl;
        std::cout << "初始参数:" << std::endl;
        std::cout << "- 候选k组数量: " << originalGroups.size() << std::endl;
        std::cout << "- j组数量: " << jCombinations.size() << std::endl;
        std::cout << "- s子集数量: " << sSubsets.size() << std::endl;
        std::cout << "- Beam宽度: " << BEAM_WIDTH << std::endl;
        
        // 预处理：筛选k组
        std::vector<std::vector<int>> groups;
        std::map<std::vector<std::vector<int>>, int> coverageCount;  // 记录每个k组覆盖的s子集数量
        
        for (const auto& group : originalGroups) {
            int coverCount = 0;
            std::set<std::vector<int>> coveredS;
            
            // 计算该k组覆盖了多少个s子集
            for (const auto& sSubset : sSubsets) {
                if (m_setOps->contains(group, sSubset)) {
                    coverCount++;
                    coveredS.insert(sSubset);
                }
            }
            
            // 如果只覆盖1个或更少的s子集，直接跳过
            if (coverCount <= 1) continue;
            
            // 检查是否与已有组覆盖完全相同
            bool isDuplicate = false;
            for (const auto& [existingCovered, count] : coverageCount) {
                if (coveredS == std::set<std::vector<int>>(existingCovered.begin(), existingCovered.end())) {
                    isDuplicate = true;
                    break;
                }
            }
            
            if (!isDuplicate) {
                groups.push_back(group);
                coverageCount[std::vector<std::vector<int>>(coveredS.begin(), coveredS.end())] = coverCount;
            }
        }
        
        // 1. 分析s子集中元素的频率
        std::map<int, int> elementFrequency;
        std::map<int, std::vector<int>> elementToSSubsets;  // 元素到包含它的s子集的映射
        for (size_t i = 0; i < sSubsets.size(); ++i) {
            for (int elem : sSubsets[i]) {
                elementFrequency[elem]++;
                elementToSSubsets[elem].push_back(i);
            }
        }
        
        // 候选状态结构
        struct State {
            std::vector<std::vector<int>> selectedGroups;  // 已选择的k组
            std::set<int> coveredSSubsets;                 // 已覆盖的s子集索引
            std::map<std::string, int> jCoverage;          // j组覆盖次数 (使用string作为key)
            double score;                                  // 状态得分
            
            bool operator<(const State& other) const {
                return score < other.score;
            }
        };
        
        // 辅助函数：将 jGroup 转成 string 作为 map key
        auto vecToStr = [](const std::vector<int>& vec) {
            std::string key;
            for (int val : vec) {
                key += std::to_string(val) + ",";
            }
            return key;
        };
        
        // 初始化beam：使用Warm Start
        std::vector<State> beam;
        
        // 改进Warm Start策略：找到覆盖最多j组的s子集
        std::map<std::vector<int>, int> sToJCount;
        for (const auto& s : sSubsets) {
            auto it = sToJMap_.find(s);
            if (it != sToJMap_.end()) {
                sToJCount[s] = it->second.size();
            }
        }
        
        auto maxSJ = std::max_element(sToJCount.begin(), sToJCount.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        // 使用覆盖最多j组的s子集对应的k组作为warm start
        if (maxSJ != sToJCount.end()) {
            for (const auto& group : groups) {
                if (m_setOps->contains(group, maxSJ->first)) {
                    State warmStartState{{}, {}, {}, 0.0};
                    warmStartState.selectedGroups.push_back(group);
                    
                    // 更新覆盖信息
                    for (size_t i = 0; i < sSubsets.size(); ++i) {
                        if (m_setOps->contains(group, sSubsets[i])) {
                            warmStartState.coveredSSubsets.insert(i);
                        }
                    }
                    
                    // 初始化j组覆盖
                    for (const auto& j : jCombinations) {
                        warmStartState.jCoverage[vecToStr(j)] = 0;
                    }
                    
                    beam.push_back(warmStartState);
                    break;
                }
            }
        }
        
        // 找到高中心度s的union生成的k组
        std::set<int> centralElements;
        for (const auto& [elem, freq] : elementFrequency) {
            if (freq > sSubsets.size() / 3) {  // 选择出现在超过1/3 s子集中的元素
                centralElements.insert(elem);
            }
        }
        std::vector<int> centralGroup(centralElements.begin(), centralElements.end());
        if (!groups.empty() && centralGroup.size() > groups[0].size()) {
            centralGroup.resize(groups[0].size());
        }
        
        // 初始化beam with warm start
        State initialState1{{}, {}, {}, 0.0};
        if (!centralGroup.empty()) {
            initialState1.selectedGroups.push_back(centralGroup);
        }
        
        beam.push_back(initialState1);
        
        // 更新初始状态的覆盖信息
        for (auto& state : beam) {
            for (const auto& group : state.selectedGroups) {
                for (size_t i = 0; i < sSubsets.size(); ++i) {
                    if (m_setOps->contains(group, sSubsets[i])) {
                        state.coveredSSubsets.insert(i);
                    }
                }
            }
            // 初始化j组覆盖
            for (const auto& j : jCombinations) {
                state.jCoverage[vecToStr(j)] = 0;
            }
        }
        
        // 记录最佳解
        State bestState = beam[0];
        double bestScore = 0.0;
        int bestCoveredCount = 0;
        
        // 迭代构建解
        for (int iter = 0; iter < MAX_ITERATIONS && !beam.empty(); ++iter) {
            std::cout << "\n迭代 " << iter + 1 << ":" << std::endl;
            std::cout << "当前beam大小: " << beam.size() << std::endl;
            
            std::vector<State> candidates;
            
            // 对beam中的每个状态扩展
            for (const auto& state : beam) {
                std::cout << "处理状态 - 已选组数: " << state.selectedGroups.size() 
                         << ", 已覆盖s子集数: " << state.coveredSSubsets.size() << std::endl;
                
                // 检查是否所有s子集都被覆盖
                if (state.coveredSSubsets.size() == sSubsets.size()) {
                    std::cout << "找到完全覆盖解!" << std::endl;
                    return state.selectedGroups;
                }
                
                int candidatesBeforeThisState = candidates.size();
                
                // 构造候选k组
                for (const auto& group : groups) {
                    // 跳过已选的组
                    if (std::find(state.selectedGroups.begin(), 
                                state.selectedGroups.end(), group) != state.selectedGroups.end()) {
                        continue;
                    }
                    
                    // 创建新状态
                    State newState = state;
                    newState.selectedGroups.push_back(group);
                    
                    // 计算这个k组能覆盖哪些s子集
                    int newlyCoveredS = 0;
                    std::set<int> commonElements;
                    double jaccardSum = 0.0;
                    
                    for (size_t i = 0; i < sSubsets.size(); ++i) {
                        if (state.coveredSSubsets.count(i) > 0) continue;
                        
                        if (m_setOps->contains(group, sSubsets[i])) {
                            newState.coveredSSubsets.insert(i);
                            newlyCoveredS++;
                            
                            // 更新j组覆盖
                            auto it = sToJMap_.find(sSubsets[i]);
                            if (it != sToJMap_.end()) {
                                for (const auto& j : it->second) {
                                    newState.jCoverage[vecToStr(j)]++;
                                }
                            }
                            
                            // 计算与其他已覆盖s子集的Jaccard相似度
                            for (int coveredIdx : state.coveredSSubsets) {
                                jaccardSum += m_setOps->calculateJaccardSimilarity(sSubsets[i], sSubsets[coveredIdx]);
                            }
                            
                            // 收集公共元素
                            if (commonElements.empty()) {
                                commonElements.insert(sSubsets[i].begin(), sSubsets[i].end());
                            } else {
                                std::set<int> intersection;
                                std::set<int> current(sSubsets[i].begin(), sSubsets[i].end());
                                std::set_intersection(
                                    commonElements.begin(), commonElements.end(),
                                    current.begin(), current.end(),
                                    std::inserter(intersection, intersection.begin())
                                );
                                commonElements = intersection;
                            }
                        }
                    }

                    std::cout << "  尝试添加组 - 新覆盖s子集数: " << newlyCoveredS;

                    // 计算新覆盖的j组数量
                    int newlyCoveredJ = 0;
                    
                    // 遍历所有j组
                    for (const auto& jGroup : jCombinations) {
                        std::string jKey = vecToStr(jGroup);
                        
                        // 如果这个j组已经被覆盖，跳过
                        auto coverageIt = state.jCoverage.find(jKey);
                        if (coverageIt != state.jCoverage.end() && coverageIt->second > 0) {
                            continue;
                        }
                        
                        // 检查j组的每个s子集是否被覆盖
                        auto jToSIt = jToSMap_.find(jGroup);
                        if (jToSIt == jToSMap_.end()) {
                            continue;
                        }
                        
                        bool isJGroupCovered = false;
                        for (const auto& s : jToSIt->second) {
                            // 检查新组是否覆盖这个s子集
                            if (m_setOps->contains(group, s)) {
                                isJGroupCovered = true;
                                break;
                            }
                            
                            // 检查已有组是否覆盖这个s子集
                            for (const auto& existingGroup : state.selectedGroups) {
                                if (m_setOps->contains(existingGroup, s)) {
                                    isJGroupCovered = true;
                                    break;
                                }
                            }
                            if (isJGroupCovered) break;
                        }
                        
                        if (isJGroupCovered) {
                            newlyCoveredJ++;
                            newState.jCoverage[jKey] = 1;
                        }
                    }

                    // 检查是否所有j组都被覆盖
                    bool allJCovered = true;
                    int uncoveredJCount = 0;
                    for (const auto& jGroup : jCombinations) {
                        std::string jKey = vecToStr(jGroup);
                        auto it = newState.jCoverage.find(jKey);
                        if (it == newState.jCoverage.end() || it->second == 0) {
                            allJCovered = false;
                            uncoveredJCount++;
                        }
                    }

                    // 更新评分函数
                    double coverageRatio = static_cast<double>(newState.coveredSSubsets.size()) / sSubsets.size();
                    double jCoverageRatio = static_cast<double>(jCombinations.size() - uncoveredJCount) / jCombinations.size();
                    
                    // 计算多样性得分 - 重用之前的jaccardSum变量
                    double diversityScore = 1.0;
                    if (newState.coveredSSubsets.size() > 1) {
                        int comparisons = 0;
                        for (auto it1 = newState.coveredSSubsets.begin(); it1 != newState.coveredSSubsets.end(); ++it1) {
                            auto it2 = it1;
                            ++it2;
                            for (; it2 != newState.coveredSSubsets.end(); ++it2) {
                                jaccardSum += m_setOps->calculateJaccardSimilarity(sSubsets[*it1], sSubsets[*it2]);
                                comparisons++;
                            }
                        }
                        if (comparisons > 0) {
                            diversityScore = 1.0 - (jaccardSum / comparisons);
                        }
                    }
                    
                    // 计算新状态的评分
                    newState.score = 
                        coverageRatio * 100 +                // s子集覆盖率
                        jCoverageRatio * 200 +              // j组覆盖率（加大权重）
                        newlyCoveredJ * 50 +                // 新覆盖的j组数量
                        newlyCoveredS * 30 +                // 新覆盖的s子集数量
                        diversityScore * 20 -               // 多样性得分
                        std::abs(static_cast<double>(3 - static_cast<int>(newState.selectedGroups.size()))) * 30;  // 组数接近3的奖励

                    // 如果已经选择了3个组但仍有未覆盖的j组，显著降低评分
                    if (newState.selectedGroups.size() >= 3 && !allJCovered) {
                        newState.score -= uncoveredJCount * 100;  // 每个未覆盖的j组都会降低评分
                    }

                    // 添加详细的调试信息
                    std::cout << "  状态评估:" << std::endl;
                    std::cout << "    - 已选组数: " << newState.selectedGroups.size() << std::endl;
                    std::cout << "    - s子集覆盖率: " << (coverageRatio * 100) << "%" << std::endl;
                    std::cout << "    - j组覆盖率: " << (jCoverageRatio * 100) << "%" << std::endl;
                    std::cout << "    - 未覆盖j组数: " << uncoveredJCount << std::endl;
                    std::cout << "    - 新覆盖j组数: " << newlyCoveredJ << std::endl;
                    std::cout << "    - 新覆盖s子集数: " << newlyCoveredS << std::endl;
                    std::cout << "    - 多样性得分: " << diversityScore << std::endl;
                    std::cout << "    - 最终评分: " << newState.score << std::endl;

                    candidates.push_back(newState);
                    
                    // 更新最佳解
                    if (newState.coveredSSubsets.size() > bestCoveredCount || 
                        (newState.coveredSSubsets.size() == bestCoveredCount && 
                         newState.score > bestScore)) {
                        bestScore = newState.score;
                        bestState = newState;
                        bestCoveredCount = newState.coveredSSubsets.size();
                    }
                }
                
                std::cout << "本状态产生的新候选数: " << (candidates.size() - candidatesBeforeThisState) << std::endl;
            }
            
            std::cout << "本轮产生的总候选数: " << candidates.size() << std::endl;
            
            // 如果没有新的候选状态，终止搜索
            if (candidates.empty()) {
                std::cout << "没有新的候选状态，终止搜索" << std::endl;
                break;
            }
            
            // 选择最好的BEAM_WIDTH个状态
            std::sort(candidates.begin(), candidates.end(), 
                     [](const State& a, const State& b) { return a.score > b.score; });
            
            beam.clear();
            for (size_t i = 0; i < std::min(static_cast<size_t>(BEAM_WIDTH), candidates.size()); ++i) {
                beam.push_back(candidates[i]);
            }
            
            std::cout << "更新后的beam大小: " << beam.size() << std::endl;
            
            // 更新最佳解
            if (!candidates.empty() && 
                (candidates[0].coveredSSubsets.size() > bestCoveredCount || 
                 (candidates[0].coveredSSubsets.size() == bestCoveredCount && 
                  candidates[0].score > bestScore))) {
                bestScore = candidates[0].score;
                bestState = candidates[0];
                bestCoveredCount = candidates[0].coveredSSubsets.size();
                std::cout << "更新最佳解 - 覆盖数: " << bestCoveredCount 
                         << ", 分数: " << bestScore 
                         << ", 组数: " << bestState.selectedGroups.size() << std::endl;
            }
        }
        
        // Final check: ensure all j groups are covered
        bool allJGroupsCoveredFinal = true;
        for (const auto& jGroup : jCombinations) {
            std::string jKey = vecToStr(jGroup);
            if (bestState.jCoverage.find(jKey) == bestState.jCoverage.end() ||
                bestState.jCoverage.at(jKey) == 0) {
                allJGroupsCoveredFinal = false;
                break;
            }
        }

        if (!allJGroupsCoveredFinal) {
            std::cout << "[Error] Final solution does NOT cover all j groups!" << std::endl;
            return {};  // return empty to signal failure
        }

        return bestState.selectedGroups;
    }

    DetailedSolution solve(
        int m,
        int n,
        const std::vector<int>& samples,
        int k,
        int s,
        int j
    ) override {
        auto startTime = std::chrono::steady_clock::now();
        
        // 1. 生成组合并建立映射关系
        auto combinations = generateCombinations(m, n, samples, k, s, j);
        
        // 2. 使用preprocessor处理生成的数据
        auto preprocessResult = m_preprocessor->preprocess(
            samples,
            n,
            j,
            s,
            k,
            CoverageMode::CoverMinOneS,  // 先传入mode
            1,  // 再传入minCoverageCount
            combinations.allSSubsets,
            combinations.jCombinations,
            combinations.sToJMap,
            combinations.jToSMap
        );
        
        // 更新覆盖映射信息
        sToJMap_ = preprocessResult.selectedSToJMap;
        
        // 3. 执行选择过程，生成最终的k集合
        auto selectedGroups = performSelection(
            combinations.groups,
            combinations.jCombinations,
            preprocessResult.selectedSSubsets,
            j,
            s
        );
        
        // 4. 计算覆盖率
        // 为每个j组合生成对应的s子集集合
        std::vector<std::vector<std::vector<int>>> sSubsetsForJ;
        for (const auto& jGroup : combinations.jCombinations) {
            auto sSubsetsForThisJ = m_combGen->generate(jGroup, s);
            sSubsetsForJ.push_back(sSubsetsForThisJ);
        }
        
        auto coverageResult = m_covCalc->calculateCoverage(
            selectedGroups,
            combinations.jCombinations,
            sSubsetsForJ,  // 使用正确的类型：每个j组合的s子集集合
            CoverageMode::CoverMinOneS,
            1  // 最小覆盖数为1
        );
        
        // 5. 准备并返回最终解决方案
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);
        
        DetailedSolution solution;
        solution.status = Status::Success;
        solution.groups = selectedGroups;
        solution.coverageRatio = coverageResult.coverage_ratio;
        solution.totalGroups = static_cast<int>(selectedGroups.size());
        solution.computationTime = duration.count();
        solution.isOptimal = coverageResult.coverage_ratio >= 1.0;
        solution.message = "Mode A solver completed successfully";
        
        return solution;
    }
};

std::shared_ptr<ModeASolver> createModeASolver(
    std::shared_ptr<CombinationGenerator> combGen,
    std::shared_ptr<SetOperations> setOps,
    std::shared_ptr<CoverageCalculator> covCalc,
    const Config& config
) {
    return std::make_shared<ModeASetCoverSolverImpl>(combGen, setOps, covCalc, config);
}

} // namespace core_algo


