#include "test_reporter.hpp"

namespace core_algo {
namespace utils {

void TestReporter::addResult(const TestResult& result) {
    results_.push_back(result);
}

void TestReporter::clearResults() {
    results_.clear();
}

std::string TestReporter::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string TestReporter::formatDuration(double milliseconds) const {
    std::stringstream ss;
    if (milliseconds < 1.0) {
        ss << std::fixed << std::setprecision(3) << milliseconds << " ms";
    } else if (milliseconds < 1000.0) {
        ss << std::fixed << std::setprecision(2) << milliseconds << " ms";
    } else {
        ss << std::fixed << std::setprecision(2) << (milliseconds / 1000.0) << " s";
    }
    return ss.str();
}

std::string TestReporter::formatSet(const std::vector<int>& set) const {
    std::stringstream ss;
    ss << "{";
    for (size_t i = 0; i < set.size(); ++i) {
        if (i > 0) ss << ", ";
        ss << set[i];
    }
    ss << "}";
    return ss.str();
}

std::string TestReporter::formatSets(const std::vector<std::vector<int>>& sets) const {
    std::stringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < sets.size(); ++i) {
        ss << "    " << formatSet(sets[i]);
        if (i < sets.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "]";
    return ss.str();
}

void TestReporter::generateReport(const std::string& outputPath) const {
    std::ofstream report(outputPath);
    if (!report.is_open()) {
        std::cerr << "Failed to open report file: " << outputPath << std::endl;
        return;
    }

    // 写入报告头部
    report << "测试报告\n";
    report << "生成时间: " << getCurrentTimestamp() << "\n\n";

    // 统计信息
    int totalTests = results_.size();
    int passedTests = 0;
    double totalDuration = 0.0;
    for (const auto& result : results_) {
        if (result.passed) passedTests++;
        totalDuration += result.duration;
    }

    // 写入总结
    report << "测试总结:\n";
    report << "总测试数: " << totalTests << "\n";
    report << "通过测试: " << passedTests << "\n";
    report << "失败测试: " << (totalTests - passedTests) << "\n";
    report << "总执行时间: " << formatDuration(totalDuration) << "\n\n";

    // 写入详细结果
    report << "详细测试结果:\n";
    report << std::string(80, '-') << "\n";
    for (const auto& result : results_) {
        report << "测试套件: " << result.testSuite << "\n";
        report << "测试名称: " << result.testName << "\n";
        
        // 输出测试参数
        report << "测试参数:\n";
        for (const auto& param : result.parameters) {
            report << "  " << param.first << ": " << param.second << "\n";
        }
        
        // 输出模式类型
        if (!result.modeType.empty()) {
            report << "测试模式: " << result.modeType << "\n";
        }

        // 输出性能指标
        report << "性能指标:\n";
        report << "  覆盖率: " << std::fixed << std::setprecision(2) << (result.metrics.coverageRatio * 100) << "%\n";
        report << "  平均组大小: " << result.metrics.avgGroupSize << "\n";
        report << "  组间相似度: " << result.metrics.interGroupSimilarity << "\n";
        report << "  总组合数: " << result.metrics.totalCombinations << "\n";

        // 输出生成的集合
        if (!result.generatedSets.empty()) {
            report << "生成的集合:\n" << formatSets(result.generatedSets) << "\n";
        }

        // 输出其他信息
        if (!result.additionalInfo.empty()) {
            report << "补充信息: " << result.additionalInfo << "\n";
        }

        report << "执行时间: " << formatDuration(result.duration) << "\n";
        report << "测试结果: " << (result.passed ? "通过" : "失败") << "\n";
        if (!result.message.empty()) {
            report << "详细信息: " << result.message << "\n";
        }
        report << std::string(80, '-') << "\n";
    }

    report.close();
}

} // namespace utils
} // namespace core_algo 