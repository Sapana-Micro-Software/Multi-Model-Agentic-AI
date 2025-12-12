/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 */

#include "thread_pool.hpp"

namespace utils {

ThreadPool::ThreadPool(size_t num_threads)
    : stop_(false)
{
    for (size_t i = 0; i < num_threads; ++i) {
        threads_.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::worker() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
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
}

size_t ThreadPool::getQueueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (std::thread& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

} // namespace utils

