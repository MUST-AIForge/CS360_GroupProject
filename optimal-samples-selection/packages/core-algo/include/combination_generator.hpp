#pragma once

#include <vector>
#include <memory>
#include "types.hpp"

namespace core_algo {

class CombinationGenerator {
public:
    // 迭代器类型定义
    class Iterator;
    using iterator = Iterator;
    
    virtual ~CombinationGenerator() = default;
    
    // 生成随机样本
    virtual std::vector<int> generateRandomSamples(int m, int n) const = 0;
    
    // 生成所有r-元组合
    virtual std::vector<std::vector<int>> generate(
        const std::vector<int>& elements,
        int r
    ) const = 0;
    
    // 获取迭代器（延迟生成）
    virtual std::unique_ptr<Iterator> getIterator(
        const std::vector<int>& elements,
        int r
    ) = 0;
    
    // 获取总组合数
    virtual size_t getCombinationCount(
        size_t n,
        size_t r
    ) const = 0;
    
    // 并行生成接口
    virtual std::vector<std::vector<int>> generateParallel(
        const std::vector<int>& elements,
        int r,
        int threadCount
    ) = 0;
    
    // 新增：生成j组合及其对应的s子集
    virtual std::pair<std::vector<std::vector<int>>, std::vector<std::vector<std::vector<int>>>> 
    generateJCombinationsAndSSubsets(int m, int n, int j, int s) = 0;
    
    // 为单个j组合生成所有可能的s子集
    virtual std::vector<std::vector<int>> generateSSubsetsForJCombination(
        const std::vector<int>& j_combination,
        int s
    ) const = 0;

    // 生成组合缓存
    virtual CombinationCache generateCombinations(
        const std::vector<int>& samples,
        int j,
        int s
    ) const = 0;
    
    // 工厂方法
    static std::unique_ptr<CombinationGenerator> create(const Config& config = Config());
};

// 迭代器接口
class CombinationGenerator::Iterator {
public:
    virtual ~Iterator() = default;
    
    // 迭代器操作
    virtual bool hasNext() const = 0;
    virtual std::vector<int> next() = 0;
    virtual void reset() = 0;
    
    // 获取当前进度（0.0 - 1.0）
    virtual double getProgress() const = 0;
};

} // namespace core_algo 