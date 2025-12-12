#include "agent_manager.hpp"
#include <algorithm>

namespace agent {

AgentManager::AgentManager() = default;

AgentManager::~AgentManager() {
    stopAllMessageProcessing();
}

bool AgentManager::createAgent(const AgentConfig& config) {
    return createAgent(config.id, config.model_path, config.trace_limit, config.memory_size);
}

bool AgentManager::createAgent(const std::string& agent_id,
                               const std::string& model_path,
                               size_t trace_limit,
                               size_t memory_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if agent already exists
    if (agents_.find(agent_id) != agents_.end()) {
        return false;
    }
    
    // Create agent
    auto agent = std::make_unique<Agent>(agent_id, model_path, trace_limit, memory_size);
    agent->setAgentManager(this);
    
    // Create message queue for agent
    auto queue = std::make_unique<communication::MessageQueue>();
    
    // Initialize agent
    if (!agent->initialize()) {
        return false;
    }
    
    // Store agent and queue
    agents_[agent_id] = std::move(agent);
    agent_queues_[agent_id] = std::move(queue);
    
    // Register with router
    router_.registerAgent(agent_id, agent_queues_[agent_id].get());
    
    // Start message processing
    startMessageProcessing(agent_id);
    
    return true;
}

bool AgentManager::removeAgent(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto agent_it = agents_.find(agent_id);
    if (agent_it == agents_.end()) {
        return false;
    }
    
    // Stop message processing
    stopMessageProcessing(agent_id);
    
    // Unregister from router
    router_.unregisterAgent(agent_id);
    
    // Remove agent and queue
    agents_.erase(agent_it);
    agent_queues_.erase(agent_id);
    
    return true;
}

Agent* AgentManager::getAgent(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = agents_.find(agent_id);
    if (it != agents_.end()) {
        return it->second.get();
    }
    
    return nullptr;
}

bool AgentManager::hasAgent(const std::string& agent_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return agents_.find(agent_id) != agents_.end();
}

std::vector<std::string> AgentManager::getAgentIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> ids;
    ids.reserve(agents_.size());
    
    for (const auto& pair : agents_) {
        ids.push_back(pair.first);
    }
    
    return ids;
}

size_t AgentManager::getAgentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return agents_.size();
}

bool AgentManager::routeMessage(const communication::Message& message) {
    return router_.routeMessage(message);
}

std::string AgentManager::submitTask(const std::string& agent_id, const std::string& task_keyword) {
    Agent* agent = getAgent(agent_id);
    if (!agent) {
        return "Error: Agent not found: " + agent_id;
    }
    
    return agent->processTask(task_keyword);
}

void AgentManager::startMessageProcessing(const std::string& agent_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (thread_running_[agent_id]) {
        return; // Already running
    }
    
    thread_running_[agent_id] = true;
    message_threads_[agent_id] = std::thread(&AgentManager::messageProcessingLoop, this, agent_id);
}

void AgentManager::stopMessageProcessing(const std::string& agent_id) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!thread_running_[agent_id]) {
            return; // Not running
        }
        
        thread_running_[agent_id] = false;
        
        // Close queue to wake up thread
        auto queue_it = agent_queues_.find(agent_id);
        if (queue_it != agent_queues_.end()) {
            queue_it->second->close();
        }
    }
    
    // Wait for thread to finish
    auto thread_it = message_threads_.find(agent_id);
    if (thread_it != message_threads_.end()) {
        if (thread_it->second.joinable()) {
            thread_it->second.join();
        }
        message_threads_.erase(thread_it);
    }
}

void AgentManager::stopAllMessageProcessing() {
    std::vector<std::string> agent_ids = getAgentIds();
    for (const auto& agent_id : agent_ids) {
        stopMessageProcessing(agent_id);
    }
}

void AgentManager::messageProcessingLoop(const std::string& agent_id) {
    Agent* agent = getAgent(agent_id);
    if (!agent) {
        return;
    }
    
    auto queue_it = agent_queues_.find(agent_id);
    if (queue_it == agent_queues_.end()) {
        return;
    }
    
    communication::MessageQueue* queue = queue_it->second.get();
    communication::Message message;
    
    while (thread_running_[agent_id]) {
        if (queue->pop(message)) {
            agent->handleMessage(message);
        }
    }
}

bool AgentManager::initializeAgent(const std::string& agent_id) {
    Agent* agent = getAgent(agent_id);
    if (!agent) {
        return false;
    }
    
    return agent->initialize();
}

} // namespace agent

