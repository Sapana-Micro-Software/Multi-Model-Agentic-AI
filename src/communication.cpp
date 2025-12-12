#include "communication.hpp"
#include <algorithm>

namespace communication {

// MessageQueue Implementation

void MessageQueue::push(const Message& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!closed_) {
        queue_.push(message);
        condition_.notify_one();
    }
}

bool MessageQueue::pop(Message& message) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    condition_.wait(lock, [this] {
        return !queue_.empty() || closed_;
    });
    
    if (closed_ && queue_.empty()) {
        return false;
    }
    
    message = queue_.front();
    queue_.pop();
    return true;
}

bool MessageQueue::tryPop(Message& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return false;
    }
    
    message = queue_.front();
    queue_.pop();
    return true;
}

bool MessageQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t MessageQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void MessageQueue::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    closed_ = true;
    condition_.notify_all();
}

bool MessageQueue::isClosed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

// MessageRouter Implementation

void MessageRouter::registerAgent(const std::string& agent_id, MessageQueue* queue) {
    std::lock_guard<std::mutex> lock(mutex_);
    agent_queues_[agent_id] = queue;
}

void MessageRouter::unregisterAgent(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    agent_queues_.erase(agent_id);
}

bool MessageRouter::routeMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = agent_queues_.find(message.to_agent_id);
    if (it != agent_queues_.end()) {
        it->second->push(message);
        return true;
    }
    
    return false;
}

void MessageRouter::broadcast(const Message& message, const std::string& exclude_agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pair : agent_queues_) {
        if (pair.first != exclude_agent_id) {
            pair.second->push(message);
        }
    }
}

} // namespace communication

