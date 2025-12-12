/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * No MIT License
 * 
 * Fault tolerance: Retry mechanisms, error recovery, graceful degradation
 */

#ifndef RETRY_HPP
#define RETRY_HPP

#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <memory>

namespace fault_tolerance {

/**
 * Retry policy configuration
 */
struct RetryPolicy {
    size_t max_attempts;
    std::chrono::milliseconds initial_delay;
    std::chrono::milliseconds max_delay;
    double backoff_multiplier;
    bool exponential_backoff;
    
    RetryPolicy()
        : max_attempts(3)
        , initial_delay(100)
        , max_delay(5000)
        , backoff_multiplier(2.0)
        , exponential_backoff(true)
    {}
};

/**
 * Retry executor with configurable policies
 */
template<typename Result>
class RetryExecutor {
public:
    RetryExecutor(RetryPolicy policy = RetryPolicy())
        : policy_(policy)
        , total_attempts_(0)
        , successful_attempts_(0)
        , failed_attempts_(0)
    {}
    
    /**
     * Execute function with retry
     * @param func Function to execute
     * @param should_retry Predicate to determine if retry is needed
     * @return Result of execution
     */
    Result execute(std::function<Result()> func,
                  std::function<bool(const Result&)> should_retry = nullptr);
    
    /**
     * Execute with exception handling
     */
    Result executeWithException(std::function<Result()> func);
    
    /**
     * Get statistics
     */
    size_t getTotalAttempts() const { return total_attempts_; }
    size_t getSuccessfulAttempts() const { return successful_attempts_; }
    size_t getFailedAttempts() const { return failed_attempts_; }

private:
    RetryPolicy policy_;
    std::atomic<size_t> total_attempts_;
    std::atomic<size_t> successful_attempts_;
    std::atomic<size_t> failed_attempts_;
    
    void waitBeforeRetry(size_t attempt);
};

/**
 * Circuit breaker for fault tolerance
 */
class CircuitBreaker {
public:
    enum class State { CLOSED, OPEN, HALF_OPEN };
    
    CircuitBreaker(size_t failure_threshold = 5,
                  std::chrono::milliseconds timeout = std::chrono::milliseconds(60000));
    
    /**
     * Execute function with circuit breaker
     */
    template<typename Result>
    Result execute(std::function<Result()> func);
    
    /**
     * Get current state
     */
    State getState() const { return state_; }
    
    /**
     * Reset circuit breaker
     */
    void reset();

private:
    size_t failure_threshold_;
    std::chrono::milliseconds timeout_;
    std::atomic<State> state_;
    std::atomic<size_t> failure_count_;
    std::chrono::system_clock::time_point last_failure_time_;
    std::mutex mutex_;
    
    void recordSuccess();
    void recordFailure();
    bool shouldAttemptReset();
};

/**
 * Error recovery manager
 */
class ErrorRecoveryManager {
public:
    ErrorRecoveryManager();
    
    /**
     * Register recovery strategy for error type
     */
    void registerRecovery(const std::string& error_type,
                         std::function<bool()> recovery_func);
    
    /**
     * Attempt recovery
     */
    bool attemptRecovery(const std::string& error_type);
    
    /**
     * Graceful degradation
     */
    template<typename Result>
    Result executeWithFallback(std::function<Result()> primary,
                              std::function<Result()> fallback);

private:
    std::map<std::string, std::function<bool()>> recovery_strategies_;
    std::mutex mutex_;
};

// Template implementations
template<typename Result>
Result RetryExecutor<Result>::execute(std::function<Result()> func,
                                     std::function<bool(const Result&)> should_retry) {
    Result result;
    size_t attempt = 0;
    
    while (attempt < policy_.max_attempts) {
        total_attempts_++;
        attempt++;
        
        try {
            result = func();
            
            if (!should_retry || !should_retry(result)) {
                successful_attempts_++;
                return result;
            }
            
            failed_attempts_++;
            
            if (attempt < policy_.max_attempts) {
                waitBeforeRetry(attempt);
            }
        } catch (...) {
            failed_attempts_++;
            if (attempt >= policy_.max_attempts) {
                throw;
            }
            waitBeforeRetry(attempt);
        }
    }
    
    return result;
}

template<typename Result>
Result RetryExecutor<Result>::executeWithException(std::function<Result()> func) {
    return execute(func, [](const Result&) { return false; });
}

template<typename Result>
void RetryExecutor<Result>::waitBeforeRetry(size_t attempt) {
    std::chrono::milliseconds delay = policy_.initial_delay;
    
    if (policy_.exponential_backoff) {
        delay = std::chrono::milliseconds(
            static_cast<long>(policy_.initial_delay.count() * 
                            std::pow(policy_.backoff_multiplier, attempt - 1))
        );
    }
    
    delay = std::min(delay, policy_.max_delay);
    std::this_thread::sleep_for(delay);
}

template<typename Result>
Result CircuitBreaker::execute(std::function<Result()> func) {
    if (state_ == State::OPEN) {
        if (shouldAttemptReset()) {
            state_ = State::HALF_OPEN;
        } else {
            throw std::runtime_error("Circuit breaker is OPEN");
        }
    }
    
    try {
        Result result = func();
        recordSuccess();
        return result;
    } catch (...) {
        recordFailure();
        throw;
    }
}

template<typename Result>
Result ErrorRecoveryManager::executeWithFallback(std::function<Result()> primary,
                                                std::function<Result()> fallback) {
    try {
        return primary();
    } catch (...) {
        try {
            return fallback();
        } catch (...) {
            throw;
        }
    }
}

} // namespace fault_tolerance

#endif // RETRY_HPP

