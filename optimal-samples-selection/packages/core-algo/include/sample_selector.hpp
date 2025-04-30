#pragma once

#include "types.hpp"
#include <vector>
#include <string>
#include <memory>

namespace core_algo {

class CombinationGenerator;
class SetOperations;

// 使用types.hpp中的CoverageMode定义
// 在types.hpp中添加AUTO模式
// enum class CoverageMode {
//     CoverMinOneS,   // Mode A: 每个组至少覆盖s个样本
//     CoverMinNS,     // Mode B: 至少N个组覆盖s个样本
//     CoverAllS,      // Mode C: 所有组都覆盖s个样本
//     AUTO            // 自动模式：根据n参数自动选择合适的模式
// };

class SampleSelector {
public:
    /**
     * @brief 寻找最优的样本分组
     * @param m 总样本数
     * @param n 当mode为AUTO时，用于自动选择模式：
     *         - n=1: 自动选择Mode A
     *         - n>=所有可能组合数: 自动选择Mode C
     *         - 1<n<所有可能组合数: 自动选择Mode B，并将n作为最小覆盖数
     * @param samples 样本列表
     * @param k 每组样本数
     * @param j 组数
     * @param s 每组需要覆盖的样本数
     * @param mode 覆盖模式
     * @param N 当mode为CoverMinNS时的最小覆盖数
     * @return Solution 包含最优分组结果的解决方案
     */
    Solution findOptimalGroups(
        int m, int n, const std::vector<int>& samples,
        int k, int j, int s, CoverageMode mode = CoverageMode::UNION, int N = 0);

private:
    // 内部辅助函数
    bool validateInputs(int m, int n, const std::vector<int>& samples, 
                       int k, int j, int s, CoverageMode mode, int N);

    // Mode A: 至少覆盖一个s的实现
    void findGroupsForModeA(
        const std::vector<int>& samples,
        int k,
        int s,
        CombinationGenerator* combGen,
        SetOperations* coverageCalc,
        Solution& solution
    );

    // Mode B: 至少覆盖N个s的实现
    void findGroupsForModeB(
        const std::vector<int>& samples,
        int k,
        int s,
        int N,
        CombinationGenerator* combGen,
        SetOperations* coverageCalc,
        Solution& solution
    );

    // Mode C: 覆盖所有s的实现
    void findGroupsForModeC(
        const std::vector<int>& samples,
        int k,
        int s,
        CombinationGenerator* combGen,
        SetOperations* coverageCalc,
        Solution& solution
    );
};

} // namespace core_algo 