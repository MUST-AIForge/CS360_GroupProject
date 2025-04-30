#pragma once

#include <vector>
#include <thread>
#include <future>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace optimal_samples {
namespace utils {

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency())
        : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        if (stop_ && tasks_.empty()) {
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (std::thread& worker : workers_) {
            worker.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return res;
    }

    size_t getThreadCount() const {
        return workers_.size();
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_;
};

class ParallelExecutor {
public:
    template<typename Iterator, typename Function>
    static void parallelFor(
        Iterator begin,
        Iterator end,
        Function fn,
        size_t numThreads = std::thread::hardware_concurrency()
    ) {
        if (begin == end) return;

        size_t totalSize = std::distance(begin, end);
        size_t batchSize = (totalSize + numThreads - 1) / numThreads;

        std::vector<std::future<void>> futures;
        ThreadPool pool(numThreads);

        for (size_t i = 0; i < numThreads; ++i) {
            auto start = begin + std::min(i * batchSize, totalSize);
            auto stop = begin + std::min((i + 1) * batchSize, totalSize);
            if (start == stop) break;

            futures.push_back(pool.enqueue([start, stop, &fn]() {
                std::for_each(start, stop, fn);
            }));
        }

        for (auto& future : futures) {
            future.get();
        }
    }

    template<typename Container>
    static void parallelSort(
        Container& container,
        size_t numThreads = std::thread::hardware_concurrency()
    ) {
        if (container.size() < 2) return;

        size_t mid = container.size() / 2;
        Container left(container.begin(), container.begin() + mid);
        Container right(container.begin() + mid, container.end());

        auto future = std::async(std::launch::async, [&]() {
            parallelSort(left, numThreads / 2);
        });

        parallelSort(right, numThreads / 2);
        future.wait();

        std::inplace_merge(
            container.begin(),
            container.begin() + mid,
            container.end()
        );
    }
};

} // namespace utils
} // namespace optimal_samples 