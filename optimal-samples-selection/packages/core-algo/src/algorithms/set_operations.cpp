#include "set_operations.hpp"
#include "timer.hpp"
#include <algorithm>
#include <unordered_set>
#include <numeric>
#include <future>
#include <thread>
#include <iostream>

namespace core_algo {

namespace {
    class SetOperationsImpl : public SetOperations {
    private:
        Config config;
        const size_t NUM_THREADS = std::thread::hardware_concurrency();
        static constexpr size_t LARGE_SET_THRESHOLD = 5000;
        
        // 缓存常用的集合操作结果
        mutable std::unordered_map<size_t, std::vector<int>> unionCache;
        mutable std::unordered_map<size_t, std::vector<int>> intersectionCache;
        
        size_t calculateSetsHash(const std::vector<std::vector<int>>& sets) const {
            size_t hash = 0;
            for (const auto& set : sets) {
                for (int elem : set) {
                    hash = hash * 31 + static_cast<size_t>(elem);
                }
                hash = hash * 31 + set.size();
            }
            return hash;
        }

        // 并发处理大型集合
        std::vector<int> parallelSetOperation(
            const std::vector<std::vector<int>>& sets,
            bool isUnion
        ) const {
            const size_t batchSize = (sets.size() + NUM_THREADS - 1) / NUM_THREADS;
            std::vector<std::future<std::unordered_set<int>>> futures;

            for (size_t i = 0; i < sets.size(); i += batchSize) {
                size_t end = std::min(i + batchSize, sets.size());
                futures.push_back(std::async(std::launch::async,
                    [this, &sets, i, end]() {
                        return this->processSetsBatch(sets, i, end);
                    }));
            }

            std::unordered_set<int> result;
            if (isUnion) {
                for (auto& future : futures) {
                    auto partialResult = future.get();
                    result.insert(partialResult.begin(), partialResult.end());
                }
            } else {  // intersection
                if (!futures.empty()) {
                    result = futures[0].get();
                    for (size_t i = 1; i < futures.size(); ++i) {
                        auto partialResult = futures[i].get();
                        std::unordered_set<int> tempSet;
                        for (const auto& elem : result) {
                            if (partialResult.find(elem) != partialResult.end()) {
                                tempSet.insert(elem);
                            }
                        }
                        result = std::move(tempSet);
                    }
                }
            }

            std::vector<int> vectorResult(result.begin(), result.end());
            std::sort(vectorResult.begin(), vectorResult.end());
            return vectorResult;
        }

        std::unordered_set<int> processSetsBatch(
            const std::vector<std::vector<int>>& sets,
            size_t startIdx,
            size_t endIdx
        ) const {
            std::unordered_set<int> result;
            result.reserve(LARGE_SET_THRESHOLD);  // 预分配空间
            for (size_t i = startIdx; i < endIdx; ++i) {
                result.insert(sets[i].begin(), sets[i].end());
            }
            return result;
        }

    public:
        explicit SetOperationsImpl(const Config& config = Config()) 
            : config(config) {}

        std::vector<int> getUnion(const std::vector<std::vector<int>>& sets) const override {
            Timer timer("计算并集");
            if (sets.empty()) return {};

            // 检查缓存
            size_t setsHash = calculateSetsHash(sets);
            auto cacheIt = unionCache.find(setsHash);
            if (cacheIt != unionCache.end()) {
                return cacheIt->second;
            }

            std::vector<int> result;
            if (sets.size() > NUM_THREADS && 
                std::accumulate(sets.begin(), sets.end(), size_t(0),
                    [](size_t sum, const std::vector<int>& set) { 
                        return sum + set.size(); 
                    }) > LARGE_SET_THRESHOLD) {
                result = parallelSetOperation(sets, true);
            } else {
                std::unordered_set<int> unionSet;
                unionSet.reserve(LARGE_SET_THRESHOLD);
            for (const auto& set : sets) {
                    unionSet.insert(set.begin(), set.end());
                }
                result.assign(unionSet.begin(), unionSet.end());
                std::sort(result.begin(), result.end());
            }

            // 更新缓存
            unionCache[setsHash] = result;
            return result;
        }

        std::vector<int> getIntersection(const std::vector<std::vector<int>>& sets) const override {
            Timer timer("计算交集");
            if (sets.empty()) return {};
            
            // 检查缓存
            size_t setsHash = calculateSetsHash(sets);
            auto cacheIt = intersectionCache.find(setsHash);
            if (cacheIt != intersectionCache.end()) {
                return cacheIt->second;
            }

            std::vector<int> result;
            if (sets.size() > NUM_THREADS && 
                std::accumulate(sets.begin(), sets.end(), size_t(0),
                    [](size_t sum, const std::vector<int>& set) { 
                        return sum + set.size(); 
                    }) > LARGE_SET_THRESHOLD) {
                result = parallelSetOperation(sets, false);
            } else {
                if (sets.size() == 1) {
                    result = sets[0];
                    std::sort(result.begin(), result.end());
                } else {
                    std::unordered_set<int> intersectionSet(sets[0].begin(), sets[0].end());
            for (size_t i = 1; i < sets.size(); ++i) {
                std::unordered_set<int> currentSet(sets[i].begin(), sets[i].end());
                std::unordered_set<int> tempSet;
                        tempSet.reserve(intersectionSet.size());
                
                        for (const int& element : intersectionSet) {
                    if (currentSet.find(element) != currentSet.end()) {
                        tempSet.insert(element);
                            }
                        }
                        intersectionSet = std::move(tempSet);
                    }
                    result.assign(intersectionSet.begin(), intersectionSet.end());
                    std::sort(result.begin(), result.end());
                }
            }

            // 更新缓存
            intersectionCache[setsHash] = result;
            return result;
        }

        std::vector<int> getDifference(
            const std::vector<int>& setA,
            const std::vector<int>& setB
        ) const override {
            std::unordered_set<int> setB_hash(setB.begin(), setB.end());
            std::vector<int> result;
            
            for (const auto& element : setA) {
                if (setB_hash.find(element) == setB_hash.end()) {
                    result.push_back(element);
                }
            }
            
            std::sort(result.begin(), result.end());
            return result;
        }

        std::vector<int> getSymmetricDifference(
            const std::vector<int>& setA,
            const std::vector<int>& setB
        ) const override {
            std::unordered_set<int> resultSet;
            std::unordered_set<int> setB_hash(setB.begin(), setB.end());
            
            // 添加在A中但不在B中的元素
            for (const auto& element : setA) {
                if (setB_hash.find(element) == setB_hash.end()) {
                    resultSet.insert(element);
                }
            }
            
            // 添加在B中但不在A中的元素
            std::unordered_set<int> setA_hash(setA.begin(), setA.end());
            for (const auto& element : setB) {
                if (setA_hash.find(element) == setA_hash.end()) {
                    resultSet.insert(element);
                }
            }
            
            std::vector<int> result(resultSet.begin(), resultSet.end());
            std::sort(result.begin(), result.end());
            return result;
        }

        double calculateJaccardSimilarity(
            const std::vector<int>& setA,
            const std::vector<int>& setB
        ) const override {
            if (setA.empty() && setB.empty()) return 1.0;
            if (setA.empty() || setB.empty()) return 0.0;

            std::unordered_set<int> unionSet;
            std::unordered_set<int> intersectionSet;
            std::unordered_set<int> setB_hash(setB.begin(), setB.end());

            // 计算并集大小
            unionSet.insert(setA.begin(), setA.end());
            unionSet.insert(setB.begin(), setB.end());

            // 计算交集大小
            for (const auto& element : setA) {
                if (setB_hash.find(element) != setB_hash.end()) {
                    intersectionSet.insert(element);
                }
            }

            return static_cast<double>(intersectionSet.size()) / unionSet.size();
        }

        bool contains(
            const std::vector<int>& container,
            const std::vector<int>& subset
        ) const override {
            if (subset.empty()) return true;
            if (container.empty()) return false;
            if (subset.size() > container.size()) return false;

            // 使用哈希集来提高查找效率
            std::unordered_set<int> containerSet(container.begin(), container.end());
            
            // 检查subset中的每个元素是否都在container中
            return std::all_of(subset.begin(), subset.end(),
                [&containerSet](int elem) { return containerSet.find(elem) != containerSet.end(); });
        }

        std::vector<int> getAllCombinations(const std::vector<std::vector<int>>& sets) const override {
            Timer timer("计算所有组合");
            if (sets.empty()) return {};

            std::unordered_set<int> allElements;
                    for (const auto& set : sets) {
                allElements.insert(set.begin(), set.end());
                    }
            
            std::vector<int> result(allElements.begin(), allElements.end());
            std::sort(result.begin(), result.end());
            return result;
        }

        bool isValid(const std::vector<int>& set) const override {
            if (set.empty()) return false;
            
            // 检查是否有重复元素
            std::unordered_set<int> uniqueElements(set.begin(), set.end());
            if (uniqueElements.size() != set.size()) return false;
            
            // 检查是否有负数
            return std::all_of(set.begin(), set.end(), 
                [](int x) { return x >= 0; });
        }

        std::vector<int> normalize(const std::vector<int>& set) const override {
            if (set.empty()) return {};
            
            // 去重并排序
            std::unordered_set<int> uniqueElements;
            std::vector<int> result;
            
            for (int element : set) {
                if (element >= 0 && uniqueElements.insert(element).second) {
                    result.push_back(element);
                }
            }
            
            std::sort(result.begin(), result.end());
            return result;
        }

        void clearCache() override {
            Timer timer("清理缓存");
            unionCache.clear();
            intersectionCache.clear();
            unionCache.reserve(1000);  // 预分配合适的空间
            intersectionCache.reserve(1000);
        }
    };
} // anonymous namespace

// 工厂方法实现
std::unique_ptr<SetOperations> SetOperations::create(const Config& config) {
    return std::make_unique<SetOperationsImpl>(config);
}

} // namespace core_algo 