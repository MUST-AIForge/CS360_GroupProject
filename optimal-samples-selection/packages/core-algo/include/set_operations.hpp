#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <unordered_set>

namespace core_algo {

class SetOperations {
public:
    virtual ~SetOperations() = default;

    // 检查集合是否有效（元素唯一且有序）
    virtual bool isValid(
        const std::vector<int>& set
    ) const = 0;

    // 将无序集合转换为有序集合
    virtual std::vector<int> normalize(
        const std::vector<int>& set
    ) const = 0;

    // 计算覆盖率
    virtual double calculateCoverage(
        const std::vector<std::vector<int>>& universe,
        const std::vector<std::vector<int>>& selectedSets
    ) const = 0;

    // 计算多个集合的并集
    virtual std::vector<int> getUnion(
        const std::vector<std::vector<int>>& sets
    ) const = 0;

    // 计算多个集合的交集
    virtual std::vector<int> getIntersection(
        const std::vector<std::vector<int>>& sets
    ) const = 0;

    // 计算两个集合的差集 (A - B)
    virtual std::vector<int> getDifference(
        const std::vector<int>& setA,
        const std::vector<int>& setB
    ) const = 0;

    // 计算两个集合的对称差
    virtual std::vector<int> getSymmetricDifference(
        const std::vector<int>& setA,
        const std::vector<int>& setB
    ) const = 0;

    // 计算两个集合的Jaccard相似度
    virtual double calculateJaccardSimilarity(
        const std::vector<int>& setA,
        const std::vector<int>& setB
    ) const = 0;

    // 检查一个集合是否包含另一个集合
    virtual bool contains(
        const std::vector<int>& container,
        const std::vector<int>& subset
    ) const = 0;

    // 获取所有可能的组合
    virtual std::vector<int> getAllCombinations(
        const std::vector<std::vector<int>>& sets
    ) const = 0;

    // 清理缓存
    virtual void clearCache() = 0;

    // 构建覆盖矩阵
    virtual std::vector<std::vector<bool>> buildCoverageMatrix(
        const std::vector<std::vector<int>>& groups,
        const std::vector<std::vector<int>>& targetGroups
    ) const = 0;

    // 工厂方法
    static std::unique_ptr<SetOperations> create(const Config& config = Config());
};

} // namespace core_algo 