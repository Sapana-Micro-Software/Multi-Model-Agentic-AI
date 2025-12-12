/*
 * Copyright (C) 2025, Shyamal Chandra
 * No MIT License
 */

#include "retry.hpp"
#include <cmath>

namespace fault_tolerance {

CircuitBreaker::CircuitBreaker(size_t failure_threshold,
                              std::chrono::milliseconds timeout)
    : failure_threshold_(failure_threshold)
    , timeout_(timeout)
    , state_(State::CLOSED)
    , failure_count_(0)
{
}

void CircuitBreaker::recordSuccess() {
    std::lock_guard<std::mutex> lock(mutex_);
    failure_count_ = 0;
    if (state_ == State::HALF_OPEN) {
        state_ = State::CLOSED;
    }
}

void CircuitBreaker::recordFailure() {
    std::lock_guard<std::mutex> lock(mutex_);
    failure_count_++;
    last_failure_time_ = std::chrono::system_clock::now();
    
    if (failure_count_ >= failure_threshold_) {
        state_ = State::OPEN;
    }
}

bool CircuitBreaker::shouldAttemptReset() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_failure_time_);
    return elapsed >= timeout_;
}

void CircuitBreaker::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = State::CLOSED;
    failure_count_ = 0;
}

ErrorRecoveryManager::ErrorRecoveryManager() = default;

void ErrorRecoveryManager::registerRecovery(const std::string& error_type,
                                           std::function<bool()> recovery_func) {
    std::lock_guard<std::mutex> lock(mutex_);
    recovery_strategies_[error_type] = recovery_func;
}

bool ErrorRecoveryManager::attemptRecovery(const std::string& error_type) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = recovery_strategies_.find(error_type);
    if (it != recovery_strategies_.end()) {
        return it->second();
    }
    return false;
}

} // namespace fault_tolerance

