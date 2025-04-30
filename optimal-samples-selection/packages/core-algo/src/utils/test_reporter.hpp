#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <map>

namespace core_algo {
namespace utils {

struct TestResult {
    std::string testName;
    std::string testSuite;
    bool passed;
    std::string message;
    double duration;  // in milliseconds
    
    // 扩展的测试信息
    std::map<std::string, std::string> parameters;  // 所有测试参数
    std::string modeType;                          // 测试模式类型
    std::vector<std::vector<int>> generatedSets;   // 生成的集合
    std::string additionalInfo;                    // 其他补充信息
    
    // 用于记录性能指标
    struct PerformanceMetrics {
        double coverageRatio;
        double avgGroupSize;
        double interGroupSimilarity;
        int totalCombinations;
    } metrics;
};

class TestReporter {
public:
    static TestReporter& getInstance() {
        static TestReporter instance;
        return instance;
    }

    void addResult(const TestResult& result);
    void generateReport(const std::string& outputPath) const;
    void clearResults();

private:
    TestReporter() = default;
    std::vector<TestResult> results_;
    
    std::string getCurrentTimestamp() const;
    std::string formatDuration(double milliseconds) const;
    std::string formatSet(const std::vector<int>& set) const;
    std::string formatSets(const std::vector<std::vector<int>>& sets) const;
};

} // namespace utils
} // namespace core_algo 