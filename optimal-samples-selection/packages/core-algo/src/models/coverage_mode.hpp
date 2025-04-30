#pragma once

#include <string>
#include <vector>
#include "sample.hpp"

namespace optimal_samples {

/**
 * @brief 覆盖模式枚举
 */
enum class CoverageMode {
    MODE_A,     // CoverMinOneS: 每个特征至少被一个样本覆盖
    MODE_B,     // CoverMinNS: 每个特征至少被N个样本覆盖
    MODE_C      // CoverAllS: 覆盖所有可能的特征组合
};

/**
 * @brief 覆盖模式基类接口
 */
class CoverageModeBase {
public:
    virtual ~CoverageModeBase() = default;

    // 检查给定样本集是否满足覆盖要求
    virtual bool checkCoverage(const std::vector<Sample>& samples) const = 0;

    // 计算覆盖率
    virtual double calculateCoverageRate(const std::vector<Sample>& samples) const = 0;

    // 获取覆盖模式类型
    virtual CoverageMode getMode() const = 0;

    // 获取模式描述
    virtual std::string getDescription() const = 0;
};

/**
 * @brief Mode A (CoverMinOneS) 实现
 */
class CoverageModeModeA : public CoverageModeBase {
public:
    CoverageMode getMode() const override { return CoverageMode::MODE_A; }
    std::string getDescription() const override { return "每个特征至少被一个样本覆盖"; }
    
    bool checkCoverage(const std::vector<Sample>& samples) const override;
    double calculateCoverageRate(const std::vector<Sample>& samples) const override;
};

/**
 * @brief Mode B (CoverMinNS) 实现
 */
class CoverageModeModeB : public CoverageModeBase {
public:
    explicit CoverageModeModeB(size_t minCoverage) : minCoverage_(minCoverage) {}

    CoverageMode getMode() const override { return CoverageMode::MODE_B; }
    std::string getDescription() const override { 
        return "每个特征至少被" + std::to_string(minCoverage_) + "个样本覆盖"; 
    }

    bool checkCoverage(const std::vector<Sample>& samples) const override;
    double calculateCoverageRate(const std::vector<Sample>& samples) const override;

private:
    size_t minCoverage_;  // 最小覆盖次数要求
};

/**
 * @brief Mode C (CoverAllS) 实现
 */
class CoverageModeModeC : public CoverageModeBase {
public:
    explicit CoverageModeModeC(size_t combinationSize) : combinationSize_(combinationSize) {}

    CoverageMode getMode() const override { return CoverageMode::MODE_C; }
    std::string getDescription() const override { 
        return "覆盖所有" + std::to_string(combinationSize_) + "个特征的组合"; 
    }

    bool checkCoverage(const std::vector<Sample>& samples) const override;
    double calculateCoverageRate(const std::vector<Sample>& samples) const override;

private:
    size_t combinationSize_;  // 特征组合的大小
};

} // namespace optimal_samples 