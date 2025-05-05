#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <algorithm>
#include <numeric>
#include <gtest/gtest.h>

#include "mode_a_solver.hpp"
#include "mode_b_solver.hpp"
#include "mode_c_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include "types.hpp"

using namespace std;
using namespace chrono;
using namespace core_algo;

void printUsage() {
    cout << "用法: simple_test [选项]\n"
         << "选项:\n"
         << "  -m <总样本数>      总样本数量 (必需)\n"
         << "  -n <选择样本数>    需要选择的样本数量 (必需)\n"
         << "  -k <组大小>        每组的大小 (必需)\n"
         << "  -j <j大小>         j大小 (必需)\n"
         << "  -s <子集大小>      子集的大小 (必需)\n"
         << "  -N <覆盖数量>      需要覆盖的不同s大小子集的数量 (必需)\n"
         << "  -mode <算法模式>   使用的算法模式 (a/b/c) (必需)\n"
         << "  -case <测试用例>   使用的测试用例编号 (1-8) (必需)\n"
         << "  -h                 显示此帮助信息\n";
}

struct Parameters {
    int m = 0;      // 总样本数
    int n = 0;      // 选择样本数
    int k = 0;      // 组大小
    int j = 0;      // j大小
    int s = 0;      // 子集大小
    int N = 0;      // 需要覆盖的子集数量
    char mode = 0;  // 算法模式
    int testCase = 0; // 测试用例编号
};

// 标准答案结构
struct StandardAnswer {
    vector<vector<string>> groups;
    int m, n, k, j, s;
    int N;  // 需要覆盖的子集数量
};

// 8个测试用例的标准答案
vector<StandardAnswer> standardAnswers = {
    // E.g. 1
    {
        {
            {"A","B","C","D","E","G"}, {"A","B","C","D","F","G"}, {"A","B","C","E","F","G"},
            {"A","B","D","E","F","G"}, {"A","C","D","E","F","G"}, {"B","C","D","E","F","G"}
        },
        45, 7, 6, 5, 5, 1
    },
    // E.g. 2
    {
        {
            {"A","B","C","D","G","H"}, {"A","B","C","E","G","H"}, {"A","B","C","F","G","H"},
            {"A","B","D","E","F","G"}, {"A","C","D","E","F","H"}, {"B","C","D","E","F","H"},
            {"C","D","E","F","G","H"}
        },
        45, 8, 6, 4, 4, 1
    },
    // E.g. 3
    {
        {
            {"A","B","C","D","E","I"}, {"A","B","C","E","G","H"}, {"A","B","C","F","H","I"},
            {"A","B","D","E","F","G"}, {"A","B","D","G","H","I"}, {"A","C","D","E","F","H"},
            {"A","C","D","F","G","I"}, {"A","E","F","G","H","I"}, {"B","C","D","F","G","H"},
            {"B","C","E","F","G","I"}, {"B","D","E","F","H","I"}, {"C","D","E","G","H","I"}
        },
        45, 9, 6, 4, 4, 1
    },
    // E.g. 4
    {
        {
            {"A","B","C","E","G","H"}, {"A","B","D","F","G","H"}, {"A","C","D","E","F","H"},
            {"B","C","D","E","F","G"}
        },
        45, 8, 6, 6, 5, 1
    },
    // E.g. 5
    {
        {
            {"A","B","C","D","E","H"}, {"A","B","C","E","F","H"}, {"A","B","C","E","G","H"},
            {"A","B","D","E","F","G"}, {"A","B","D","F","G","H"}, {"A","C","D","E","F","G"},
            {"A","D","E","F","G","H"}, {"B","C","D","E","G","H"}, {"B","C","D","F","G","H"},
            {"B","D","E","F","G","H"}
        },
        45, 8, 6, 6, 5, 4
    },
    // E.g. 6
    {
        {
            {"A","B","D","F","G","H"}, {"A","C","E","G","H","I"}, {"B","C","D","E","F","I"}
        },
        45, 9, 6, 5, 4, 1
    },
    // E.g. 7
    {
        {
            {"A","B","E","G","I","J"}, {"A","C","E","G","H","J"}, {"B","C","D","F","H","I"}
        },
        45, 10, 6, 6, 4, 1
    },
    // E.g. 8
    {
        {
            {"A","B","D","G","K","L"}, {"A","C","D","H","J","L"}, {"A","D","E","F","I","L"},
            {"B","C","G","H","J","K"}, {"B","E","F","G","I","K"}, {"C","E","F","H","I","J"}
        },
        45, 12, 6, 6, 4, 1
    }
};

Parameters parseCommandLine(int argc, char* argv[]) {
    Parameters params;
    
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "-h") {
            printUsage();
            exit(0);
        }
        
        if (i + 1 >= argc) {
            throw runtime_error("缺少参数值");
        }
        
        if (arg == "-mode") {
            params.mode = argv[i + 1][0];
            if (params.mode != 'a' && params.mode != 'b' && params.mode != 'c') {
                throw runtime_error("模式必须是 a、b 或 c");
            }
        } else {
            int value = atoi(argv[i + 1]);
            if (value <= 0) {
                throw runtime_error("参数值必须为正整数");
            }
            
            if (arg == "-m") params.m = value;
            else if (arg == "-n") params.n = value;
            else if (arg == "-k") params.k = value;
            else if (arg == "-j") params.j = value;
            else if (arg == "-s") params.s = value;
            else if (arg == "-N") params.N = value;
            else if (arg == "-case") {
                params.testCase = value;
                if (value < 1 || value > 8) {
                    throw runtime_error("测试用例编号必须在1-8之间");
                }
            }
            else throw runtime_error("未知参数: " + arg);
        }
        
        i++;
    }
    
    // 验证所有必需参数都已提供
    if (params.m == 0 || params.n == 0 || params.k == 0 || 
        params.j == 0 || params.s == 0 || params.N == 0 || 
        params.mode == 0 || params.testCase == 0) {
        throw runtime_error("缺少必需参数");
    }
    
    // 验证参数合理性
    if (params.k > params.n) {
        throw runtime_error("组大小不能大于选择样本数");
    }
    if (params.n > params.m) {
        throw runtime_error("选择样本数不能大于总样本数");
    }
    if (params.s > params.k) {
        throw runtime_error("子集大小不能大于组大小");
    }
    if (params.j > params.n) {
        throw runtime_error("j不能大于选择样本数");
    }
    
    return params;
}

// 将字母转换为数字
int letterToNum(const string& letter) {
    return letter[0] - 'A';
}

// 将数字转换为字母
string numToLetter(int num) {
    return string(1, char('A' + num));
}

// 声明compareResults函数的两个版本
bool compareResults(const vector<vector<string>>& solution_groups, const vector<vector<string>>& answer_groups);
bool compareResults(const vector<vector<int>>& solution_groups, const vector<vector<string>>& answer_groups);
    
// 实现string版本的compareResults
bool compareResults(const vector<vector<string>>& solution_groups, const vector<vector<string>>& answer_groups) {
    if (solution_groups.size() != answer_groups.size()) {
        return false;
    }

    // 创建两个集合的副本以进行排序和比较
    vector<vector<string>> sorted_solution = solution_groups;
    vector<vector<string>> sorted_answer = answer_groups;

    // 对每个组内的元素进行排序
    for (auto& group : sorted_solution) {
        sort(group.begin(), group.end());
    }
    for (auto& group : sorted_answer) {
        sort(group.begin(), group.end());
    }
    
    // 对组列表进行排序
    sort(sorted_solution.begin(), sorted_solution.end());
    sort(sorted_answer.begin(), sorted_answer.end());

    // 比较排序后的结果
    return sorted_solution == sorted_answer;
}
    
// 实现int版本的compareResults
bool compareResults(const vector<vector<int>>& solution_groups, const vector<vector<string>>& answer_groups) {
    vector<vector<string>> converted_solution;
    // 将int类型转换为string类型
    for (const auto& group : solution_groups) {
        vector<string> converted_group;
        for (int num : group) {
            converted_group.push_back(numToLetter(num));
        }
        converted_solution.push_back(converted_group);
    }
    return compareResults(converted_solution, answer_groups);
}

vector<vector<string>> generateSubsets(const vector<string>& group) {
    vector<vector<string>> result;
    int n = group.size();
    
    // 生成所有可能的子集
    for (int i = 0; i < (1 << n); i++) {
        vector<string> subset;
        for (int j = 0; j < n; j++) {
            if (i & (1 << j)) {
                subset.push_back(group[j]);
            }
        }
        if (!subset.empty()) {
            result.push_back(subset);
            }
        }
    
    return result;
}

void runAllTestCases() {
    Config config;
    auto combGen = std::shared_ptr<CombinationGenerator>(CombinationGenerator::create(config).release());
    auto setOps = std::shared_ptr<SetOperations>(SetOperations::create(config).release());
    auto covCalc = std::shared_ptr<CoverageCalculator>(CoverageCalculator::create(config).release());
    
    try {
        for (size_t i = 0; i < standardAnswers.size(); ++i) {
            cout << "\n测试用例 " << (i + 1) << ":\n";
            const auto& answer = standardAnswers[i];
            
            Parameters params;
            params.m = answer.m;
            params.n = answer.n;
            params.k = answer.k;
            params.j = answer.j;
            params.s = answer.s;
            params.N = answer.N;
            params.testCase = i + 1;
            
            vector<int> samples(params.n);
            for (int j = 0; j < params.n; ++j) {
                samples[j] = j;
            }
            
            // 根据测试用例选择合适的模式
            if (i < 3) { // 例子1-3使用Mode C
                params.mode = 'c';
            } else if (i == 4) { // 例子5使用Mode B
                params.mode = 'b';
            } else { // 例子4,6,7,8使用Mode A
                params.mode = 'a';
            }
            
            cout << "\n使用模式 " << params.mode << ":\n";
            
            DetailedSolution solution;
            if (params.mode == 'a') {
                auto solver = createModeASetCoverSolver(combGen, setOps, covCalc, config);
                if (!solver) {
                    throw runtime_error("无法创建Mode A求解器");
                }
                solution = solver->solve(params.m, params.n, samples, params.k, params.s, params.j);
            } else if (params.mode == 'b') {
                auto modeBsolver = createModeBSetCoverSolver(combGen, setOps, covCalc, config);
                if (!modeBsolver) {
                    throw runtime_error("无法创建Mode B求解器");
                }
                solution = modeBsolver->solve(params.m, params.n, samples, params.k, params.s, params.j);
            } else {
                auto modeCsolver = createModeCSetCoverSolver(combGen, setOps, covCalc, config);
                if (!modeCsolver) {
                    throw runtime_error("无法创建Mode C求解器");
                }
                solution = modeCsolver->solve(params.m, params.n, samples, params.k, params.s, params.j);
            }
            
            // 直接使用solver返回的结果
            if (solution.status == Status::Success) {
            cout << "状态: " << static_cast<int>(solution.status) << "\n";
            cout << "生成组数: " << solution.groups.size() << "\n";
            cout << "覆盖率: " << fixed << setprecision(2) 
                 << solution.coverageRatio * 100 << "%\n";
            
                // 打印生成的组
                cout << "\n生成的组:\n";
                for (const auto& group : solution.groups) {
                    cout << "{ ";
                    for (int elem : group) {
                        cout << numToLetter(elem) << " ";
                    }
                    cout << "}\n";
                }
                
                // 打印标准答案
                cout << "\n标准答案:\n";
                for (const auto& group : answer.groups) {
                    cout << "{ ";
                    for (const auto& elem : group) {
                        cout << elem << " ";
                    }
                    cout << "}\n";
                }
                
                // 比较结果
                bool matches = compareResults(solution.groups, answer.groups);
                cout << "\n结果比较: " << (matches ? "匹配" : "不匹配") << "\n";
            }
        }
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
    }
}

void runPerformanceTest(const Parameters& params) {
    Config config;
    auto combGen = std::shared_ptr<CombinationGenerator>(CombinationGenerator::create(config).release());
    auto setOps = std::shared_ptr<SetOperations>(SetOperations::create(config).release());
    auto covCalc = std::shared_ptr<CoverageCalculator>(CoverageCalculator::create(config).release());
    
    try {
        vector<int> samples(params.n);
        for (int i = 0; i < params.n; ++i) {
            samples[i] = i;
        }
        
        DetailedSolution solution;
        if (params.mode == 'a') {
            auto solver = createModeASetCoverSolver(combGen, setOps, covCalc, config);
            if (!solver) {
                throw runtime_error("无法创建Mode A求解器");
            }
            solution = solver->solve(params.m, params.n, samples, params.k, params.s, params.j);
        } else if (params.mode == 'b') {
            auto modeBsolver = createModeBSetCoverSolver(combGen, setOps, covCalc, config);
            if (!modeBsolver) {
                throw runtime_error("无法创建Mode B求解器");
            }
            solution = modeBsolver->solve(params.m, params.n, samples, params.k, params.s, params.j);
        } else {
            auto modeCsolver = createModeCSetCoverSolver(combGen, setOps, covCalc, config);
            if (!modeCsolver) {
                throw runtime_error("无法创建Mode C求解器");
            }
            solution = modeCsolver->solve(params.m, params.n, samples, params.k, params.s, params.j);
        }
        
        // 打印结果
        cout << "\n求解结果:\n";
        cout << "使用模式: " << params.mode << "\n";
        cout << "状态: " << static_cast<int>(solution.status) << "\n";
        cout << "生成组数: " << solution.groups.size() << "\n";
        
        // 直接使用solver返回的结果
        if (solution.status == Status::Success) {
        cout << "覆盖率: " << fixed << setprecision(2) 
             << solution.coverageRatio * 100 << "%\n";
        }
        
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1) {
            // 如果没有参数，运行所有测试用例
            runAllTestCases();
            return 0;
        }
        
        // 否则按原来的方式处理命令行参数
        Parameters params = parseCommandLine(argc, argv);
        
        // 运行性能测试
        runPerformanceTest(params);
        
        return 0;
    } catch (const exception& e) {
        cerr << "错误: " << e.what() << "\n";
        printUsage();
        return 1;
    }
}

TEST(SimpleTest, CompareResultsTest) {
    // 测试用例1：完全相同的组
    vector<vector<string>> solution1 = {{"A", "B"}, {"C", "D"}};
    vector<vector<string>> answer1 = {{"B", "A"}, {"D", "C"}};
    EXPECT_TRUE(compareResults(solution1, answer1));

    // 测试用例2：不同顺序但内容相同的组
    vector<vector<string>> solution2 = {{"X", "Y"}, {"W", "Z"}};
    vector<vector<string>> answer2 = {{"W", "Z"}, {"X", "Y"}};
    EXPECT_TRUE(compareResults(solution2, answer2));

    // 测试用例3：不同的组
    vector<vector<string>> solution3 = {{"A", "B"}, {"C", "D"}};
    vector<vector<string>> answer3 = {{"A", "B"}, {"C", "E"}};
    EXPECT_FALSE(compareResults(solution3, answer3));

    // 测试用例4：大小不同的组
    vector<vector<string>> solution4 = {{"A", "B"}};
    vector<vector<string>> answer4 = {{"A", "B"}, {"C", "D"}};
    EXPECT_FALSE(compareResults(solution4, answer4));
}