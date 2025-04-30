#ifdef EMSCRIPTEN
#include <emscripten/bind.h>
#include "sample_selector.hpp"

using namespace emscripten;
using namespace core_algo;

// 辅助函数：将 C++ vector<vector<int>> 转换为 JS Array
val convertGroupsToJS(const std::vector<std::vector<int>>& groups) {
    val result = val::array();
    for (const auto& group : groups) {
        val jsGroup = val::array();
        for (int sample : group) {
            jsGroup.call<void>("push", sample);
        }
        result.call<void>("push", jsGroup);
    }
    return result;
}

// 辅助函数：将 JS Array 转换为 C++ vector<int>
std::vector<int> convertJSArrayToVector(const val& array) {
    std::vector<int> result;
    auto length = array["length"].as<unsigned>();
    result.reserve(length);
    for (unsigned i = 0; i < length; ++i) {
        result.push_back(array[i].as<int>());
    }
    return result;
}

// 包装 Solution 结构体，使其对 JavaScript 友好
struct JSSolution {
    val groups;              // JavaScript 数组
    int totalGroups;
    double computationTime;
    bool isOptimal;

    // 默认构造函数
    JSSolution() 
        : groups(val::array())
        , totalGroups(0)
        , computationTime(0.0)
        , isOptimal(false) {}

    // 从 Solution 构造
    JSSolution(const Solution& solution)
        : groups(convertGroupsToJS(solution.groups))
        , totalGroups(solution.totalGroups)
        , computationTime(solution.computationTime)
        , isOptimal(solution.isOptimal) {}

    // Getter 方法
    val getGroups() const { return groups; }
    int getTotalGroups() const { return totalGroups; }
    double getComputationTime() const { return computationTime; }
    bool getIsOptimal() const { return isOptimal; }
};

// 包装主要函数
JSSolution findOptimalGroups(
    int m, val samples, int k, int j, int s,
    CoverageMode mode, int N = 1) {
    
    std::vector<int> samplesVec = convertJSArrayToVector(samples);
    int n = samplesVec.size();
    
    SampleSelector selector;
    Solution solution = selector.findOptimalGroups(
        m, n, samplesVec, k, j, s, mode, N);
    
    return JSSolution(solution);
}

// 注册绑定
EMSCRIPTEN_BINDINGS(core_algo_module) {
    enum_<CoverageMode>("CoverageMode")
        .value("CoverMinOneS", CoverageMode::CoverMinOneS)
        .value("CoverMinNS", CoverageMode::CoverMinNS)
        .value("CoverAllS", CoverageMode::CoverAllS)
        ;

    class_<JSSolution>("Solution")
        .constructor<>()
        .function("getGroups", &JSSolution::getGroups)
        .function("getTotalGroups", &JSSolution::getTotalGroups)
        .function("getComputationTime", &JSSolution::getComputationTime)
        .function("getIsOptimal", &JSSolution::getIsOptimal)
        ;

    function("findOptimalGroups", &findOptimalGroups);
}

#else
// 非 Emscripten 环境下的空实现
#include "sample_selector.hpp"
#endif 