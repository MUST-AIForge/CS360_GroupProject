#include "combination_generator.hpp"
#include <algorithm>
#include <thread>
#include <future>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include <random>
#include <iostream>

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
        mutable std::mt19937 m_rng;  // 随机数生成器
        
        // 预分配的内存池
        class MemoryPool {
        private:
            mutable std::vector<std::vector<int>> m_pool;
            mutable size_t m_currentIndex;
            mutable size_t m_blockSize;  // 每个块的大小
            static constexpr size_t BLOCK_SIZE = 1024;  // 块大小
            
        public:
            MemoryPool() : m_currentIndex(0), m_blockSize(0) {}
            
            void initialize(size_t size, size_t r) const {
                m_blockSize = r;
                // 计算需要的块数
                size_t numBlocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
                // 预分配内存池
                m_pool.clear();
                m_pool.reserve(numBlocks * BLOCK_SIZE);
                
                // 初始化第一个块
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    m_pool.emplace_back(r);
                }
                m_currentIndex = 0;
            }
            
            std::vector<int>& getNext() const {
                if (m_currentIndex >= m_pool.size()) {
                    // 当前块已满，分配新块
                    size_t newBlockStart = m_pool.size();
                    size_t remainingSize = std::min(BLOCK_SIZE, m_pool.capacity() - newBlockStart);
                    for (size_t i = 0; i < remainingSize; ++i) {
                        m_pool.emplace_back(m_blockSize);
                    }
                }
                return m_pool[m_currentIndex++];
            }
            
            void reset() const {
                m_currentIndex = 0;
            }
            
            size_t size() const {
                return m_pool.size();
            }
            
            size_t capacity() const {
                return m_pool.capacity();
            }
        };
        
        class IteratorImpl : public Iterator {
        private:
            const std::vector<int>& m_elements;  // 改为引用，避免复制
            std::vector<int> m_indices;          // 使用索引而不是实际元素
            int m_r;
            bool m_hasNext;
            size_t m_total;
            size_t m_count;
            const MemoryPool* m_memoryPool;

        public:
            IteratorImpl(const std::vector<int>& elements, int r, const MemoryPool* pool = nullptr)
                : m_elements(elements)
                , m_r(r)
                , m_hasNext(true)
                , m_count(0)
                , m_memoryPool(pool)
            {
                if (r <= 0 || r > static_cast<int>(elements.size())) {
                    m_hasNext = false;
                    m_total = 0;
                    return;
                }
                
                // 初始化索引数组
                m_indices.resize(r);
                for (int i = 0; i < r; ++i) {
                    m_indices[i] = i;
                }
                
                // 预计算总组合数
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

                // 获取结果向量
                std::vector<int> result = m_memoryPool ? m_memoryPool->getNext() : std::vector<int>(m_r);
                
                // 使用索引生成组合
                for (int i = 0; i < m_r; ++i) {
                    result[i] = m_elements[m_indices[i]];
                }

                // 生成下一个索引组合
                int i = m_r - 1;
                while (i >= 0 && m_indices[i] == static_cast<int>(m_elements.size()) - (m_r - i)) {
                    --i;
                }

                if (i < 0) {
                    m_hasNext = false;
                } else {
                    ++m_indices[i];
                    for (int j = i + 1; j < m_r; ++j) {
                        m_indices[j] = m_indices[i] + (j - i);
                    }
                }

                ++m_count;
                return result;
            }

            void reset() override {
                if (m_r <= 0 || m_r > static_cast<int>(m_elements.size())) {
                    m_hasNext = false;
                    return;
                }

                // 重置索引
                m_indices.resize(m_r);
                for (int i = 0; i < m_r; ++i) {
                    m_indices[i] = i;
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

        std::vector<std::vector<int>> randomizeCombinations(
            std::vector<std::vector<int>>& combinations
        ) const {
            if (m_config.enableRandomization) {
                std::shuffle(combinations.begin(), combinations.end(), m_rng);
            }
            return combinations;
        }

        // 辅助函数：将字母转换为数字（A->1, B->2, etc.）
        int letterToNum(char letter) {
            return static_cast<int>(letter - 'A' + 1);
        }

        // 辅助函数：将数字转换为字母（1->A, 2->B, etc.）
        char numToLetter(int num) {
            return static_cast<char>('A' + num - 1);
        }

        // 新增：为单个j组合生成所有可能的s子集
        std::vector<std::vector<int>> generateSSubsetsForJCombination(
            const std::vector<int>& j_combination,
            int s
        ) const override {
            if (s <= 0 || s > static_cast<int>(j_combination.size())) {
                throw AlgorithmError("Invalid s value for generating subsets");
            }

            // 直接从j组合中生成s子集
            std::vector<std::vector<int>> s_subsets;
            std::vector<int> current_subset(s);
            std::vector<int> indices(s);
            
            // 初始化索引
            for (int i = 0; i < s; ++i) {
                indices[i] = i;
            }

            bool has_next = true;
            while (has_next) {
                // 生成当前子集
                for (int i = 0; i < s; ++i) {
                    current_subset[i] = j_combination[indices[i]];
                }
                s_subsets.push_back(current_subset);

                // 生成下一个索引组合
                int i = s - 1;
                while (i >= 0 && indices[i] == j_combination.size() - (s - i)) {
                    --i;
                }

                if (i < 0) {
                    has_next = false;
                } else {
                    ++indices[i];
                    for (int j = i + 1; j < s; ++j) {
                        indices[j] = indices[i] + (j - i);
                    }
                }
            }


            return s_subsets;
        }

        CombinationCache generateCombinations(
            const std::vector<int>& samples,
            int j,
            int s
        ) const override {
            CombinationCache cache;
            auto jGroups = generate(samples, j);
            
            for (const auto& jGroup : jGroups) {
                auto sSubsets = generateSSubsetsForJCombination(jGroup, s);
                cache.jGroupSSubsets.push_back(sSubsets);
                cache.allSSubsets.insert(
                    cache.allSSubsets.end(),
                    sSubsets.begin(),
                    sSubsets.end()
                );
            }
            return cache;
        }

    public:
        explicit CombinationGeneratorImpl(const Config& config = Config())
            : m_config(config)
        {
            // 初始化随机数生成器
            if (m_config.enableRandomization) {
                if (m_config.randomSeed == 0) {
                    std::random_device rd;
                    m_rng.seed(rd());
                } else {
                    m_rng.seed(m_config.randomSeed);
                }
            }
        }

        // 添加生成随机样本的方法
        std::vector<int> generateRandomSamples(int m, int n) const override {
            if (n > m) {
                throw std::invalid_argument("n cannot be greater than m");
            }
            
            // 生成1到m的序列
            std::vector<int> numbers(m);
            std::iota(numbers.begin(), numbers.end(), 1);
            
            // 使用随机设备生成种子
            std::random_device rd;
            std::mt19937 gen(rd());
            
            // 随机选择n个数
            std::vector<int> samples;
            samples.reserve(n);
            
            // 使用 std::shuffle 进行随机选择
            std::shuffle(numbers.begin(), numbers.end(), gen);
            samples.assign(numbers.begin(), numbers.begin() + n);
            
            // 排序以保持一致性
            std::sort(samples.begin(), samples.end());
            
            // 验证生成的样本数量
            if (samples.size() != static_cast<size_t>(n)) {
                throw std::runtime_error("生成的样本数量不正确");
            }
            
            return samples;
        }

        std::vector<std::vector<int>> generate(
            const std::vector<int>& elements,
            int r
        ) const override {
            // 检查缓存
            auto cacheKey = std::make_pair(elements.size(), r);
            if (m_config.enableCache && !m_config.enableRandomization) {
                auto it = m_combinationCache.find(cacheKey);
                if (it != m_combinationCache.end()) {
                    return it->second;
                }
            }

            // 如果 r 等于输入序列的长度，直接返回输入序列作为唯一的组合
            if (r == static_cast<int>(elements.size())) {
                std::vector<std::vector<int>> result = {elements};
                if (m_config.enableCache && !m_config.enableRandomization) {
                    m_combinationCache[cacheKey] = result;
                }
                return result;
            }

            // 预先计算组合总数
            size_t totalCombinations = getCombinationCountFast(elements.size(), r);
            if (totalCombinations == 0) return {};
            
            // 预分配结果空间
            std::vector<std::vector<int>> result;
            result.reserve(totalCombinations);  // 避免动态扩容
            
            // 预分配每个组合的空间
            for (size_t i = 0; i < totalCombinations; ++i) {
                result.emplace_back(r);  // 每个内部vector预分配r个元素的空间
            }
            
            // 初始化内存池
            m_memoryPool.initialize(totalCombinations, r);
            
            // 使用迭代器生成组合
            size_t index = 0;
            auto it = std::make_unique<IteratorImpl>(elements, r, &m_memoryPool);
            while (it->hasNext()) {
                result[index++] = it->next();  // 直接赋值到预分配的空间
            }

            // 如果启用随机化，打乱组合顺序
            if (m_config.enableRandomization) {
                randomizeCombinations(result);
            }

            // 更新缓存
            if (m_config.enableCache && !m_config.enableRandomization) {
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

            // 检查缓存（只在非随机化模式下使用缓存）
            auto cacheKey = std::make_pair(elements.size(), r);
            if (m_config.enableCache && !m_config.enableRandomization) {
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

            // 如果启用随机化，打乱最终结果
            if (m_config.enableRandomization) {
                randomizeCombinations(result);
            }

            // 更新缓存（只在非随机化模式下缓存）
            if (m_config.enableCache && !m_config.enableRandomization) {
                m_combinationCache[cacheKey] = result;
            }

            return result;
        }

        // 修改：生成j组合及其对应的s子集
        std::pair<std::vector<std::vector<int>>, std::vector<std::vector<std::vector<int>>>> 
        generateJCombinationsAndSSubsets(int m, int n, int j, int s) override {
            // 1. 使用输入样本或生成随机样本
            std::vector<int> samples;
            if (!m_config.inputSamples.empty()) {
                // 如果有输入样本，直接使用
                samples = m_config.inputSamples;
                // 如果使用字母表示，将字母转换为数字
                if (m_config.useLetter) {
                    std::vector<int> numericSamples;
                    for (int sample : samples) {
                        if (sample >= 'A' && sample <= 'Z') {
                            numericSamples.push_back(letterToNum(static_cast<char>(sample)));
                        } else {
                            numericSamples.push_back(sample);
                        }
                    }
                    samples = numericSamples;
                }
                // 确保样本数量正确
                if (samples.size() != static_cast<size_t>(n)) {
                    throw AlgorithmError("输入样本数量与n不匹配");
                }
            } else {
                // 如果没有输入样本，生成1到n的序列
                samples.resize(n);
                std::iota(samples.begin(), samples.end(), 1);  // 使用1到n的数字
            }
            
            std::cout << "\n=== 开始生成j组合和s子集 ===" << std::endl;
            std::cout << "样本: [";
            for (int sample : samples) {
                std::cout << sample << " ";
            }
            std::cout << "]" << std::endl;
            std::cout << "参数: j=" << j << ", s=" << s << std::endl;
            
            // 2. 从n个样本中生成所有大小为j的组合
            auto j_combinations = generate(samples, j);
            std::cout << "\n生成了 " << j_combinations.size() << " 个j组合" << std::endl;
            
            // 3. 对每个j组合生成其所有大小为s的子集
            std::vector<std::vector<std::vector<int>>> s_subsets;
            s_subsets.reserve(j_combinations.size());
            
            for (const auto& j_combination : j_combinations) {
                s_subsets.push_back(generateSSubsetsForJCombination(j_combination, s));
            }
            
            std::cout << "\n=== j组合和s子集生成完成 ===" << std::endl;
            std::cout << "- 总j组合数: " << j_combinations.size() << std::endl;
            std::cout << "- 每个j组合的s子集数: " << (s_subsets.empty() ? 0 : s_subsets[0].size()) << std::endl;
            
            return {j_combinations, s_subsets};
        }
    };
} // anonymous namespace

// 工厂方法实现
std::unique_ptr<CombinationGenerator> CombinationGenerator::create(const Config& config) {
    return std::make_unique<CombinationGeneratorImpl>(config);
}

} // namespace core_algo 