#include "utils/test_reporter.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <chrono>
#include <regex>

class TestEventListener : public testing::EmptyTestEventListener {
private:
    core_algo::utils::TestReporter& reporter_;
    std::chrono::high_resolution_clock::time_point testStart_;
    core_algo::utils::TestResult currentTest_;

    void parseTestInfo(const testing::TestInfo& test_info) {
        // 解析测试名称中的参数信息
        std::string fullName = test_info.name();
        
        // 添加基本测试参数
        currentTest_.parameters["TestName"] = fullName;
        currentTest_.parameters["TestSuite"] = test_info.test_suite_name();
        
        // 从测试套件名称中提取模式类型
        std::string suiteName = test_info.test_suite_name();
        if (suiteName.find("ModeA") != std::string::npos) {
            currentTest_.modeType = "Mode A";
        } else if (suiteName.find("ModeB") != std::string::npos) {
            currentTest_.modeType = "Mode B";
        } else if (suiteName.find("ModeC") != std::string::npos) {
            currentTest_.modeType = "Mode C";
        }

        // 从测试结果中获取性能指标
        const testing::TestResult* result = test_info.result();
        if (result) {
            // 初始化默认值
            currentTest_.metrics = {0.0, 0.0, 0.0, 0};
            
            // 遍历所有测试属性
            int count = result->test_property_count();
            for (int i = 0; i < count; ++i) {
                const testing::TestProperty& prop = result->GetTestProperty(i);
                std::string name = prop.key();
                std::string value = prop.value();

                try {
                    if (name == "GeneratedSets") {
                        currentTest_.additionalInfo = value;  // 保存原始字符串
                    } else if (name == "CoverageRatio") {
                        currentTest_.metrics.coverageRatio = std::stod(value);
                    } else if (name == "AvgGroupSize") {
                        currentTest_.metrics.avgGroupSize = std::stod(value);
                    } else if (name == "InterGroupSimilarity") {
                        currentTest_.metrics.interGroupSimilarity = std::stod(value);
                    } else if (name == "TotalCombinations") {
                        currentTest_.metrics.totalCombinations = std::stoi(value);
                    } else {
                        // 保存其他参数
                        currentTest_.parameters[name] = value;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing property " << name << ": " << e.what() << std::endl;
                }
            }
        }
    }

public:
    TestEventListener() : reporter_(core_algo::utils::TestReporter::getInstance()) {}

    virtual void OnTestStart(const testing::TestInfo& test_info) override {
        currentTest_ = core_algo::utils::TestResult();
        currentTest_.testSuite = test_info.test_suite_name();
        currentTest_.testName = test_info.name();
        testStart_ = std::chrono::high_resolution_clock::now();
    }

    virtual void OnTestEnd(const testing::TestInfo& test_info) override {
        auto testEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(testEnd - testStart_);
        
        currentTest_.duration = duration.count() / 1000.0; // 转换为毫秒
        currentTest_.passed = test_info.result()->Passed();
        
        if (test_info.result()->Failed()) {
            currentTest_.message = test_info.result()->GetTestPartResult(0).message();
        }
        
        parseTestInfo(test_info);
        reporter_.addResult(currentTest_);
    }
};

int main(int argc, char** argv) {
    std::string outputPath = "test_results.txt";
    if (argc > 1) {
        outputPath = argv[1];
    }

    testing::InitGoogleTest(&argc, argv);
    
    // 添加自定义监听器
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new TestEventListener());

    // 运行所有测试
    int result = RUN_ALL_TESTS();

    // 生成报告
    core_algo::utils::TestReporter::getInstance().generateReport(outputPath);

    return result;
} 