#pragma once

#include <vector>
#include <map>
#include <memory>
#include "types.hpp"  // 为了使用CoverageMode

namespace core_algo {

class CombinationGenerator;
class SetOperations;

struct PreprocessResult {
    // j组和s子集的基本信息
    std::vector<std::vector<int>> jGroups;                    // 所有j组
    std::vector<std::vector<int>> allSSubsets;               // 所有可能的s子集（从n中生成）
    std::vector<std::vector<int>> selectedSSubsets;          // 选出的Top s子集
    
    // 双向映射关系
    std::map<std::vector<int>, std::vector<std::vector<int>>> jToSMap;  // j组到其s子集的映射
    std::map<std::vector<int>, std::vector<std::vector<int>>> sToJMap;  // s子集到包含它的j组的映射
    
    // 覆盖映射信息
    std::map<std::vector<int>, int> jCoverageCount;         // 每个j组被选中的s子集覆盖次数
    std::map<std::vector<int>, std::vector<std::vector<int>>> selectedSToJMap;  // 选中的s子集到其覆盖的j组的映射
};

class Preprocessor {
public:
    Preprocessor(
        std::shared_ptr<CombinationGenerator> combGen,
        std::shared_ptr<SetOperations> setOps
    );

    PreprocessResult preprocess(
        const std::vector<int>& samples,
        int n,
        int j,
        int s,
        int k,
        CoverageMode mode,
        int minCoverageCount = 1,
        const std::vector<std::vector<int>>& existingAllSSubsets = std::vector<std::vector<int>>(),
        const std::vector<std::vector<int>>& existingJGroups = std::vector<std::vector<int>>(),
        const std::map<std::vector<int>, std::vector<std::vector<int>>>& existingSToJMap = std::map<std::vector<int>, std::vector<std::vector<int>>>(),
        const std::map<std::vector<int>, std::vector<std::vector<int>>>& existingJToSMap = std::map<std::vector<int>, std::vector<std::vector<int>>>()
    );

private:
    // 根据不同mode选择s子集的策略类
    class SelectionStrategy {
    public:
        virtual ~SelectionStrategy() = default;
        virtual std::vector<std::vector<int>> selectTopS(
            const std::vector<std::vector<int>>& allSSubsets,
            const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
            const std::vector<std::vector<int>>& jGroups,
            int n,
            int k
        ) = 0;

    protected:
        // 计算理论覆盖率：每个s子集应该覆盖多少个j组
        virtual double calculateTheoreticalCoverage(int n, int j, int s) {
            // 计算组合数：C(n-s, j-s)，表示一个s子集能命中多少个j组
            double numerator = 1.0;
            double denominator = 1.0;
            
            // 计算 C(n-s, j-s)
            for (int i = 0; i < j-s; i++) {
                numerator *= (n-s-i);
                denominator *= (i+1);
            }
            
            return numerator / denominator;
        }
    };

    // Mode A的选择策略
    class ModeAStrategy : public SelectionStrategy {
    public:
        std::vector<std::vector<int>> selectTopS(
            const std::vector<std::vector<int>>& allSSubsets,
            const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
            const std::vector<std::vector<int>>& jGroups,
            int n,
            int k
        ) override;
        
    protected:
        // 重写理论覆盖率计算函数
        double calculateTheoreticalCoverage(int n, int j, int s) override {
            // 修改后的计算逻辑
            double numerator = 1.0;
            double denominator = 1.0;
            
            // 计算 C(n-s, j-s) 但使用更激进的覆盖要求
            for (int i = 0; i < j-s; i++) {
                numerator *= (n-s-i);
                denominator *= (i+1);
            }
            
            
            return (numerator / denominator);  
        }
    };

    // Mode B的选择策略
    class ModeBStrategy : public SelectionStrategy {
    public:
        std::vector<std::vector<int>> selectTopS(
            const std::vector<std::vector<int>>& allSSubsets,
            const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
            const std::vector<std::vector<int>>& jGroups,
            int n,
            int k
        ) override;
    };

    // Mode C的选择策略
    class ModeCStrategy : public SelectionStrategy {
    public:
        std::vector<std::vector<int>> selectTopS(
            const std::vector<std::vector<int>>& allSSubsets,
            const std::map<std::vector<int>, std::vector<std::vector<int>>>& sToJMap,
            const std::vector<std::vector<int>>& jGroups,
            int n,
            int k
        ) override;
    };

    std::unique_ptr<SelectionStrategy> createStrategy(CoverageMode mode) const;

private:
    std::shared_ptr<CombinationGenerator> m_combGen;
    std::shared_ptr<SetOperations> m_setOps;
};

} // namespace core_algo 