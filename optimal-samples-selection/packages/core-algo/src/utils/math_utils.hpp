#pragma once

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>

namespace optimal_samples {
namespace utils {

class MathUtils {
public:
    /**
     * @brief 计算组合数 C(n,k)
     */
    static size_t calculateCombination(size_t n, size_t k) {
        if (k > n) return 0;
        if (k * 2 > n) k = n - k;
        if (k == 0) return 1;

        size_t result = n;
        for (size_t i = 2; i <= k; ++i) {
            result *= (n - i + 1);
            result /= i;
        }
        return result;
    }

    /**
     * @brief 计算Jaccard相似度
     */
    template<typename T>
    static double calculateJaccardSimilarity(
        const std::vector<T>& set1,
        const std::vector<T>& set2
    ) {
        if (set1.empty() && set2.empty()) return 1.0;
        if (set1.empty() || set2.empty()) return 0.0;

        size_t intersection = 0;
        size_t union_size = 0;

        std::vector<T> sorted1 = set1;
        std::vector<T> sorted2 = set2;
        std::sort(sorted1.begin(), sorted1.end());
        std::sort(sorted2.begin(), sorted2.end());

        std::vector<T> intersection_set;
        std::set_intersection(
            sorted1.begin(), sorted1.end(),
            sorted2.begin(), sorted2.end(),
            std::back_inserter(intersection_set)
        );

        std::vector<T> union_set;
        std::set_union(
            sorted1.begin(), sorted1.end(),
            sorted2.begin(), sorted2.end(),
            std::back_inserter(union_set)
        );

        return static_cast<double>(intersection_set.size()) / union_set.size();
    }

    /**
     * @brief 计算余弦相似度
     */
    template<typename T>
    static double calculateCosineSimilarity(
        const std::vector<T>& vec1,
        const std::vector<T>& vec2
    ) {
        if (vec1.size() != vec2.size()) return 0.0;
        
        double dot_product = std::inner_product(
            vec1.begin(), vec1.end(),
            vec2.begin(), 0.0
        );
        
        double norm1 = std::sqrt(std::inner_product(
            vec1.begin(), vec1.end(),
            vec1.begin(), 0.0
        ));
        
        double norm2 = std::sqrt(std::inner_product(
            vec2.begin(), vec2.end(),
            vec2.begin(), 0.0
        ));
        
        if (norm1 == 0.0 || norm2 == 0.0) return 0.0;
        return dot_product / (norm1 * norm2);
    }

    /**
     * @brief 计算平均值
     */
    template<typename T>
    static double calculateMean(const std::vector<T>& values) {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }

    /**
     * @brief 计算标准差
     */
    template<typename T>
    static double calculateStdDev(const std::vector<T>& values) {
        if (values.size() < 2) return 0.0;
        
        double mean = calculateMean(values);
        double sq_sum = std::inner_product(
            values.begin(), values.end(),
            values.begin(), 0.0,
            std::plus<>(),
            [mean](T x, T y) { return (x - mean) * (y - mean); }
        );
        
        return std::sqrt(sq_sum / (values.size() - 1));
    }
};

} // namespace utils
} // namespace optimal_samples 