#include <gtest/gtest.h>
#include "combination_generator.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <numeric>  // 添加numeric头文件

using namespace core_algo;
using namespace std::chrono;

class CombinationGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试用例开始前执行的设置
        config = Config();
        generator = CombinationGenerator::create(config);
    }

    void TearDown() override {
        // 在每个测试用例结束后执行的清理
        generator.reset();
    }

    // 辅助函数：测量执行时间
    template<typename Func>
    double measureTime(Func&& func) {
        auto start = high_resolution_clock::now();
        func();
        auto end = high_resolution_clock::now();
        return duration_cast<microseconds>(end - start).count() / 1000.0; // 转换为毫秒
    }

    // 辅助函数：格式化输出性能结果
    void printPerformanceResult(const std::string& testName, 
                              double time, 
                              size_t combinations, 
                              const std::string& config = "") {
        std::cout << "\n=== " << testName << " ===\n"
                  << "Configuration: " << config << "\n"
                  << "Time: " << std::fixed << std::setprecision(3) << time << " ms\n"
                  << "Combinations generated: " << combinations << "\n"
                  << "Average time per combination: " 
                  << (combinations > 0 ? time / combinations : 0) << " ms\n"
                  << std::string(50, '-') << std::endl;
    }

    Config config;
    std::unique_ptr<CombinationGenerator> generator;
};

TEST_F(CombinationGeneratorTest, GeneratesCorrectCombinations) {
    std::vector<int> elements = {1, 2, 3, 4};
    int k = 2;
    
    std::vector<std::vector<int>> expected = {
        {1, 2}, {1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 4}
    };
    
    auto actual = generator->generate(elements, k);
    EXPECT_EQ(actual, expected);
}

TEST_F(CombinationGeneratorTest, HandlesEmptyInput) {
    std::vector<int> elements;
    int k = 2;
    
    auto actual = generator->generate(elements, k);
    EXPECT_TRUE(actual.empty());
}

TEST_F(CombinationGeneratorTest, HandlesKEqualToN) {
    std::vector<int> elements = {1, 2, 3};
    int k = 3;
    
    auto actual = generator->generate(elements, k);
    EXPECT_EQ(actual.size(), 1);
    EXPECT_EQ(actual[0], elements);
}

TEST_F(CombinationGeneratorTest, TestIterator) {
    std::vector<int> elements = {1, 2, 3, 4};
    int k = 2;
    
    auto it = generator->getIterator(elements, k);
    std::vector<std::vector<int>> actual;
    
    while (it->hasNext()) {
        actual.push_back(it->next());
    }
    
    std::vector<std::vector<int>> expected = {
        {1, 2}, {1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 4}
    };
    
    EXPECT_EQ(actual, expected);
}

// 性能测试：基准测试
TEST_F(CombinationGeneratorTest, BenchmarkBaseline) {
    std::vector<int> elements(20);
    std::iota(elements.begin(), elements.end(), 0);  // 填充0到19
    const int k = 6;
    
    config.enableCache = false;
    config.enableParallel = false;
    generator = CombinationGenerator::create(config);
    
    double time = measureTime([&]() {
        auto result = generator->generate(elements, k);
    });
    
    size_t combinations = generator->getCombinationCount(elements.size(), k);
    printPerformanceResult("Baseline Performance", time, combinations, 
                          "Cache: Off, Parallel: Off");
}

// 性能测试：缓存效果
TEST_F(CombinationGeneratorTest, BenchmarkCaching) {
    std::vector<int> elements(20);
    std::iota(elements.begin(), elements.end(), 0);
    const int k = 6;
    
    config.enableCache = true;
    config.enableParallel = false;
    generator = CombinationGenerator::create(config);
    
    // 第一次调用（无缓存）
    double firstTime = measureTime([&]() {
        auto result = generator->generate(elements, k);
    });
    
    // 第二次调用（有缓存）
    double secondTime = measureTime([&]() {
        auto result = generator->generate(elements, k);
    });
    
    size_t combinations = generator->getCombinationCount(elements.size(), k);
    printPerformanceResult("Cache Performance (First Call)", firstTime, combinations,
                          "Cache: On, Parallel: Off");
    printPerformanceResult("Cache Performance (Second Call)", secondTime, combinations,
                          "Cache: On, Parallel: Off");
}

// 性能测试：并行处理效果
TEST_F(CombinationGeneratorTest, BenchmarkParallel) {
    std::vector<int> elements(23);  // 使用较大的输入规模
    std::iota(elements.begin(), elements.end(), 0);
    const int k = 7;
    
    // 测试不同线程数的性能
    std::vector<int> threadCounts = {1, 2, 4, 8};
    
    config.enableCache = false;
    config.enableParallel = true;
    generator = CombinationGenerator::create(config);
    
    for (int threads : threadCounts) {
        double time = measureTime([&]() {
            auto result = generator->generateParallel(elements, k, threads);
        });
        
        size_t combinations = generator->getCombinationCount(elements.size(), k);
        printPerformanceResult("Parallel Performance", time, combinations,
                             "Threads: " + std::to_string(threads));
    }
}

// 性能测试：极限情况
TEST_F(CombinationGeneratorTest, BenchmarkEdgeCases) {
    // 测试用例1：大n小k
    {
        std::vector<int> elements(25);
        std::iota(elements.begin(), elements.end(), 0);
        const int k = 4;
        
        double time = measureTime([&]() {
            auto result = generator->generate(elements, k);
        });
        
        size_t combinations = generator->getCombinationCount(elements.size(), k);
        printPerformanceResult("Edge Case - Large N, Small K", time, combinations,
                             "N: 25, K: 4");
    }
    
    // 测试用例2：n接近k
    {
        std::vector<int> elements(15);
        std::iota(elements.begin(), elements.end(), 0);
        const int k = 12;
        
        double time = measureTime([&]() {
            auto result = generator->generate(elements, k);
        });
        
        size_t combinations = generator->getCombinationCount(elements.size(), k);
        printPerformanceResult("Edge Case - N Close to K", time, combinations,
                             "N: 15, K: 12");
    }
}

// 性能测试：内存池效果
TEST_F(CombinationGeneratorTest, BenchmarkMemoryPool) {
    std::vector<int> elements(20);
    std::iota(elements.begin(), elements.end(), 0);
    const int k = 6;
    
    // 测试多次连续调用的性能
    const int iterations = 5;
    double totalTime = 0;
    
    for (int i = 0; i < iterations; ++i) {
        double time = measureTime([&]() {
            auto result = generator->generate(elements, k);
        });
        totalTime += time;
        
        size_t combinations = generator->getCombinationCount(elements.size(), k);
        printPerformanceResult("Memory Pool Performance - Iteration " + std::to_string(i + 1),
                             time, combinations);
    }
    
    std::cout << "\nAverage time over " << iterations << " iterations: "
              << (totalTime / iterations) << " ms" << std::endl;
}

// 性能测试：真实场景模拟
TEST_F(CombinationGeneratorTest, BenchmarkRealWorldScenario) {
    struct TestCase {
        int n;
        int k;
        std::string name;
        size_t cacheThreshold;    // 启用缓存的阈值
        size_t parallelThreshold; // 启用并行的阈值
        size_t hybridThreshold;   // 启用混合优化的阈值
    };

    std::vector<TestCase> testCases = {
        // 原有测试用例
        {23, 7, "Mode A测试", 300000, 100000, 1000000},
        {20, 6, "Mode B测试", 250000, 80000, 1000000},
        {15, 4, "Mode C测试", 200000, 60000, 1000000},
        // 大规模测试
        {25, 8, "大规模测试", 300000, 100000, 1000000},
        // 超大规模测试，适合混合优化
        {30, 10, "超大规模测试", 300000, 100000, 1000000},
        {35, 12, "极限规模测试", 300000, 100000, 1000000}
    };

    for (const auto& testCase : testCases) {
        std::cout << "\n=== " << testCase.name << " ===\n";
        std::vector<int> elements(testCase.n);
        std::iota(elements.begin(), elements.end(), 0);

        // 计算总组合数
        size_t totalCombinations = generator->getCombinationCount(elements.size(), testCase.k);
        std::cout << "总组合数: " << totalCombinations << "\n";

        // 基准测试（无优化）
        config.enableCache = false;
        config.enableParallel = false;
        generator = CombinationGenerator::create(config);
        
        double baselineTime = measureTime([&]() {
            auto result = generator->generate(elements, testCase.k);
        });

        // 根据组合数量决定优化策略
        bool useCache = totalCombinations >= testCase.cacheThreshold;
        bool useParallel = totalCombinations >= testCase.parallelThreshold;
        bool useHybrid = totalCombinations >= testCase.hybridThreshold;

        // 记录性能数据
        double cacheTime = 0.0, parallelTime = 0.0, hybridTime = 0.0;
        int optimalThreads = 1;
        int hybridOptimalThreads = 1;
        
        // 缓存测试
        if (useCache) {
            std::cout << "\n正在测试缓存性能（组合数 >= " << testCase.cacheThreshold << "）...\n";
            config.enableCache = true;
            config.enableParallel = false;
            generator = CombinationGenerator::create(config);
            
            const int cacheTestRuns = 3;
            std::vector<double> cacheTimes;
            
            for (int i = 0; i < cacheTestRuns; ++i) {
                double time = measureTime([&]() {
                    auto result = generator->generate(elements, testCase.k);
                });
                cacheTimes.push_back(time);
            }
            
            cacheTime = (cacheTimes[1] + cacheTimes[2]) / 2;
        }

        // 并行测试
        if (useParallel) {
            std::cout << "\n正在测试并行性能（组合数 >= " << testCase.parallelThreshold << "）...\n";
            config.enableCache = false;
            config.enableParallel = true;
            generator = CombinationGenerator::create(config);
            
            double bestTime = std::numeric_limits<double>::max();
            std::vector<int> threadCounts = {1, 2, 4, 8};
            
            for (int threads : threadCounts) {
                double time = measureTime([&]() {
                    auto result = generator->generateParallel(elements, testCase.k, threads);
                });
                
                if (time < bestTime) {
                    bestTime = time;
                    optimalThreads = threads;
                }
            }
            
            parallelTime = bestTime;
            std::cout << "并行处理最优线程数: " << optimalThreads << "\n";
        }

        // 混合优化测试（同时启用缓存和并行）
        if (useHybrid) {
            std::cout << "\n正在测试混合优化性能（组合数 >= " << testCase.hybridThreshold << "）...\n";
            config.enableCache = true;
            config.enableParallel = true;
            generator = CombinationGenerator::create(config);
            
            double bestHybridTime = std::numeric_limits<double>::max();
            std::vector<int> threadCounts = {1, 2, 4, 8};
            
            // 第一次运行用于预热缓存
            auto warmupResult = generator->generateParallel(elements, testCase.k, 1);
            
            for (int threads : threadCounts) {
                double time = measureTime([&]() {
                    auto result = generator->generateParallel(elements, testCase.k, threads);
                });
                
                if (time < bestHybridTime) {
                    bestHybridTime = time;
                    hybridOptimalThreads = threads;
                }
            }
            
            hybridTime = bestHybridTime;
            std::cout << "混合优化最优线程数: " << hybridOptimalThreads << "\n";
        }

        // 打印性能结果
        printPerformanceResult(testCase.name + " - 基准测试", baselineTime, totalCombinations, "无优化");
        
        if (useCache) {
            printPerformanceResult(testCase.name + " - 缓存优化", cacheTime, totalCombinations, 
                                 "启用缓存 (平均" + std::to_string(cacheTime) + "ms)");
            double cacheImprovement = ((baselineTime - cacheTime) / baselineTime) * 100;
            std::cout << "缓存优化提升: " << std::fixed << std::setprecision(2) << cacheImprovement << "%\n";
        }
        
        if (useParallel) {
            printPerformanceResult(testCase.name + " - 并行优化", parallelTime, totalCombinations,
                                 "启用并行 (" + std::to_string(optimalThreads) + "线程)");
            double parallelImprovement = ((baselineTime - parallelTime) / baselineTime) * 100;
            std::cout << "并行优化提升: " << std::fixed << std::setprecision(2) << parallelImprovement << "%\n";
        }

        if (useHybrid) {
            printPerformanceResult(testCase.name + " - 混合优化", hybridTime, totalCombinations,
                                 "启用混合优化 (缓存+" + std::to_string(hybridOptimalThreads) + "线程)");
            double hybridImprovement = ((baselineTime - hybridTime) / baselineTime) * 100;
            std::cout << "混合优化提升: " << std::fixed << std::setprecision(2) << hybridImprovement << "%\n";
        }

        // 选择最佳优化策略
        std::cout << "\n最佳优化策略: ";
        if (!useCache && !useParallel && !useHybrid) {
            std::cout << "数据规模较小，无需优化\n";
        } else {
            double bestTime = baselineTime;
            std::string bestStrategy = "无优化";
            
            if (useCache && cacheTime < bestTime) {
                bestTime = cacheTime;
                bestStrategy = "使用缓存";
            }
            if (useParallel && parallelTime < bestTime) {
                bestTime = parallelTime;
                bestStrategy = "使用并行 (" + std::to_string(optimalThreads) + "线程)";
            }
            if (useHybrid && hybridTime < bestTime) {
                bestTime = hybridTime;
                bestStrategy = "使用混合优化 (缓存+" + std::to_string(hybridOptimalThreads) + "线程)";
            }
            
            double improvement = ((baselineTime - bestTime) / baselineTime) * 100;
            std::cout << bestStrategy << " (性能提升: " << std::fixed << std::setprecision(2)
                     << improvement << "%)\n";
        }
        
        std::cout << std::string(50, '-') << "\n";
    }
} 