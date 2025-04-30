#pragma once
#include <vector>
#include <string>
#include "types.hpp"

namespace core_algo {

class SampleSelectorInterface {
public:
    // mode: 'a', 'b', 'c'
    // 其余参数与三种算法一致
    static DetailedSolution run(
        char mode,
        int m,
        int n,
        int k,
        int s,
        int j,
        int N,
        const std::vector<int>& samples
    );
};

} // namespace core_algo 