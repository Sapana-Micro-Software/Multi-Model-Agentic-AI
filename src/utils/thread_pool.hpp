/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * 
 * Thread pool for parallel and asynchronous execution
 */

#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>

namespace utils {

/**
 * Thread pool for parallel task execution
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    /**
     * Submit task for execution
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * Get number of threads
     */
    size_t getThreadCount() const { return threads_.size(); }
    
    /**
     * Get queue size
     */
    size_t getQueueSize() const;
    
    /**
     * Shutdown thread pool
     */
    void shutdown();

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    
    void worker();
};

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool is stopped");
        }
        tasks_.emplace([task](){ (*task)(); });
    }
    
    condition_.notify_one();
    return result;
}

} // namespace utils

#endif // THREAD_POOL_HPP

