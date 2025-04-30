#include "combination_generator.hpp"
#include <algorithm>
#include <thread>
#include <future>
#include <cmath>
#include <numeric>
#include <unordered_map>

namespace core_algo {

namespace {
    // 缓存键的哈希函数
    struct CacheKeyHash {
        size_t operator()(const std::pair<size_t, size_t>& key) const {
            return std::hash<size_t>()(key.first) ^ (std::hash<size_t>()(key.second) << 1);
        }
    };

    class CombinationGeneratorImpl : public CombinationGenerator {
    private:
        Config m_config;
        // 缓存常用的组合结果
        mutable std::unordered_map<std::pair<size_t, size_t>, std::vector<std::vector<int>>, CacheKeyHash> m_combinationCache;
        
        // 预分配的内存池
        class MemoryPool {
        private:
            std::vector<std::vector<int>> m_pool;
            size_t m_currentIndex;
            
        public:
            void initialize(size_t size, size_t r) {
                m_pool.resize(size, std::vector<int>(r));
                m_currentIndex = 0;
            }
            
            std::vector<int>& getNext() {
                if (m_currentIndex >= m_pool.size()) {
                    m_pool.emplace_back();
                }
                return m_pool[m_currentIndex++];
            }
            
            void reset() {
                m_currentIndex = 0;
            }
        };
        
        class IteratorImpl : public Iterator {
        private:
            std::vector<int> m_elements;
            std::vector<int> m_current;
            std::vector<bool> m_used;  // 用于优化查找
            int m_r;
            bool m_hasNext;
            size_t m_total;
            size_t m_count;
            MemoryPool* m_memoryPool;

        public:
            IteratorImpl(const std::vector<int>& elements, int r, MemoryPool* pool = nullptr)
                : m_elements(elements)
                , m_r(r)
                , m_used(elements.size(), false)
                , m_hasNext(true)
                , m_count(0)
                , m_memoryPool(pool)
            {
                if (r <= 0 || r > static_cast<int>(elements.size())) {
                    m_hasNext = false;
                    m_total = 0;
                    return;
                }
                
                m_current.resize(r);
                for (int i = 0; i < r; ++i) {
                    m_current[i] = i;
                    m_used[i] = true;
                }
                
                m_total = 1;
                for (int i = 0; i < r; ++i) {
                    m_total *= (elements.size() - i);
                    m_total /= (i + 1);
                }
            }

            bool hasNext() const override {
                return m_hasNext;
            }

            std::vector<int> next() override {
                if (!m_hasNext) {
                    throw AlgorithmError("No more combinations available");
                }

                if (m_elements.empty() || m_current.empty() || m_r <= 0) {
                    throw AlgorithmError("Invalid iterator state");
                }

                // 获取结果向量
                std::vector<int> result;
                if (m_memoryPool) {
                    result = m_memoryPool->getNext();
                    if (result.size() != static_cast<size_t>(m_r)) {
                        result.resize(m_r);
                    }
                } else {
                    result.resize(m_r);
                }

                // 使用安全的访问方式
                for (int i = 0; i < m_r; ++i) {
                    if (m_current[i] >= static_cast<int>(m_elements.size())) {
                        throw AlgorithmError("Index out of bounds");
                    }
                    result[i] = m_elements[m_current[i]];
                }

                // 优化的下一个组合生成
                m_hasNext = false;
                for (int i = m_r - 1; i >= 0; --i) {
                    if (m_current[i] >= static_cast<int>(m_elements.size())) {
                        continue;  // 跳过无效索引
                    }
                    m_used[m_current[i]] = false;
                    if (m_current[i] < static_cast<int>(m_elements.size()) - (m_r - i)) {
                        ++m_current[i];
                        if (m_current[i] < static_cast<int>(m_used.size())) {
                            m_used[m_current[i]] = true;
                        }
                        
                        // 优化后续位置的填充，添加边界检查
                        int nextVal = m_current[i] + 1;
                        for (int j = i + 1; j < m_r; ++j) {
                            while (nextVal < static_cast<int>(m_used.size()) && m_used[nextVal]) {
                                ++nextVal;
                            }
                            if (nextVal >= static_cast<int>(m_elements.size())) {
                                throw AlgorithmError("Invalid combination state");
                            }
                            m_current[j] = nextVal;
                            m_used[nextVal] = true;
                            ++nextVal;
                        }
                        m_hasNext = true;
                        break;
                    }
                }

                ++m_count;
                return result;
            }

            void reset() override {
                // 添加更严格的状态检查
                if (m_elements.empty()) {
                    m_hasNext = false;
                    return;
                }

                if (m_r <= 0 || m_r > static_cast<int>(m_elements.size())) {
                    m_hasNext = false;
                    return;
                }

                // 确保数据结构的大小正确
                m_current.resize(m_r);
                m_used.resize(m_elements.size());

                std::fill(m_used.begin(), m_used.end(), false);
                for (int i = 0; i < m_r; ++i) {
                    m_current[i] = i;
                    if (i < static_cast<int>(m_used.size())) {
                        m_used[i] = true;
                    }
                }
                m_hasNext = true;
                m_count = 0;
                if (m_memoryPool) {
                    m_memoryPool->reset();
                }
            }

            double getProgress() const override {
                return m_total > 0 ? static_cast<double>(m_count) / m_total : 1.0;
            }
        };

        MemoryPool m_memoryPool;

        // 优化的组合计数方法
        size_t getCombinationCountFast(size_t n, size_t r) const {
            if (r > n) return 0;
            if (r == 0 || r == n) return 1;
            if (r > n/2) r = n-r;  // 优化：使用较小的r
            
            size_t result = 1;
            for (size_t i = 0; i < r; ++i) {
                result *= (n - i);
                result /= (i + 1);
            }
            return result;
        }

    public:
        explicit CombinationGeneratorImpl(const Config& config = Config())
            : m_config(config)
        {}

        std::vector<std::vector<int>> generate(
            const std::vector<int>& elements,
            int r
        ) override {
            // 检查缓存
            auto cacheKey = std::make_pair(elements.size(), r);
            if (m_config.enableCache) {
                auto it = m_combinationCache.find(cacheKey);
                if (it != m_combinationCache.end()) {
                    return it->second;
                }
            }

            std::vector<std::vector<int>> result;
            size_t totalCombinations = getCombinationCountFast(elements.size(), r);
            if (totalCombinations == 0) return result;
            
            // 预分配结果空间
            result.reserve(totalCombinations);
            
            // 初始化内存池
            m_memoryPool.initialize(totalCombinations, r);
            
            auto it = std::make_unique<IteratorImpl>(elements, r, &m_memoryPool);
            while (it->hasNext()) {
                result.push_back(it->next());
            }

            // 更新缓存
            if (m_config.enableCache) {
                m_combinationCache[cacheKey] = result;
            }
            
            return result;
        }

        std::unique_ptr<Iterator> getIterator(
            const std::vector<int>& elements,
            int r
        ) override {
            return std::make_unique<IteratorImpl>(elements, r, &m_memoryPool);
        }

        size_t getCombinationCount(size_t n, size_t r) const override {
            return getCombinationCountFast(n, r);
        }

        std::vector<std::vector<int>> generateParallel(
            const std::vector<int>& elements,
            int r,
            int threadCount
        ) override {
            if (!m_config.enableParallel || threadCount <= 1) {
                return generate(elements, r);
            }

            // 检查缓存
            auto cacheKey = std::make_pair(elements.size(), r);
            if (m_config.enableCache) {
                auto it = m_combinationCache.find(cacheKey);
                if (it != m_combinationCache.end()) {
                    return it->second;
                }
            }

            size_t totalCombinations = getCombinationCountFast(elements.size(), r);
            if (totalCombinations == 0) return {};

            // 优化线程数
            threadCount = std::min({
                threadCount,
                static_cast<int>(std::ceil(totalCombinations / 1000.0)),
                static_cast<int>(std::thread::hardware_concurrency())
            });
            
            std::vector<std::future<std::vector<std::vector<int>>>> futures;
            std::vector<std::vector<int>> result;
            result.reserve(totalCombinations);

            // 计算每个线程的工作量
            size_t combinationsPerThread = totalCombinations / threadCount;
            size_t remainingCombinations = totalCombinations % threadCount;

            // 使用原子计数器跟踪进度
            std::atomic<size_t> completedTasks(0);

            size_t startIdx = 0;
            for (int i = 0; i < threadCount; ++i) {
                size_t count = combinationsPerThread + (i < remainingCombinations ? 1 : 0);
                
                futures.push_back(std::async(std::launch::async,
                    [this, &elements, r, startIdx, count, &completedTasks]() {
                        std::vector<std::vector<int>> threadResult;
                        threadResult.reserve(count);
                        
                        MemoryPool localPool;
                        localPool.initialize(count, r);
                        
                        auto it = std::make_unique<IteratorImpl>(elements, r, &localPool);
                        
                        // 跳过之前的组合
                        for (size_t j = 0; j < startIdx && it->hasNext(); ++j) {
                            it->next();
                        }
                        
                        // 生成当前线程的组合
                        for (size_t j = 0; j < count && it->hasNext(); ++j) {
                            threadResult.push_back(it->next());
                            ++completedTasks;
                        }
                        
                        return threadResult;
                    }
                ));
                
                startIdx += count;
            }

            // 收集结果
            for (auto& future : futures) {
                auto threadResult = future.get();
                result.insert(result.end(), 
                    std::make_move_iterator(threadResult.begin()),
                    std::make_move_iterator(threadResult.end()));
            }

            // 更新缓存
            if (m_config.enableCache) {
                m_combinationCache[cacheKey] = result;
            }

            return result;
        }
    };
} // anonymous namespace

// 工厂方法实现
std::unique_ptr<CombinationGenerator> CombinationGenerator::create(const Config& config) {
    return std::make_unique<CombinationGeneratorImpl>(config);
}

} // namespace core_algo 