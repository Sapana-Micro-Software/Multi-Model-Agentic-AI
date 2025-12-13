#include "agent_manager.hpp"
#include "verbose.hpp"
#include <algorithm>
#include <thread>
#include <chrono>
#include <stdexcept>

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
    verbose::logPhase("AGENT CREATION: " + agent_id);
    verbose::logStep("Agent Creation", 1, "Checking if agent already exists");
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if agent already exists
        if (agents_.find(agent_id) != agents_.end()) {
            verbose::log("Agent already exists, skipping", "Agent Creation");
            return false;
        }
    }
    
    verbose::logStep("Agent Creation", 2, "Creating agent object");
    // Create agent (outside lock to avoid holding it during initialization)
    auto agent = std::make_unique<Agent>(agent_id, model_path, trace_limit, memory_size);
    agent->setAgentManager(this);
    
    verbose::logStep("Agent Creation", 3, "Creating message queue");
    // Create message queue for agent
    auto queue = std::make_unique<communication::MessageQueue>();
    
    verbose::logStep("Agent Creation", 4, "Initializing agent (loading model)");
    // Initialize agent (this may take time if loading a model)
    // Do this outside the lock to avoid blocking other operations
    if (!agent->initialize()) {
        verbose::log("Agent initialization failed", "Agent Creation");
        return false;
    }
    verbose::logStep("Agent Creation", 5, "Agent initialized successfully");
    
    verbose::logStep("Agent Creation", 6, "Storing agent and queue");
    // Store agent and queue (need lock for this)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        agents_[agent_id] = std::move(agent);
        agent_queues_[agent_id] = std::move(queue);
    }
    
    verbose::logStep("Agent Creation", 7, "Registering with message router");
    // Register with router (router has its own lock, so safe)
    // Need to get queue pointer while holding lock
    communication::MessageQueue* queue_ptr = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = agent_queues_.find(agent_id);
        if (it != agent_queues_.end()) {
            queue_ptr = it->second.get();
        }
    }
    if (queue_ptr) {
        router_.registerAgent(agent_id, queue_ptr);
    }
    
    verbose::logStep("Agent Creation", 8, "Starting message processing thread");
    // Start message processing (non-blocking - creates a thread)
    // This should return immediately, thread runs in background
    // Call this OUTSIDE any lock to avoid deadlock
    startMessageProcessing(agent_id);
    verbose::logStep("Agent Creation", 9, "Message processing thread started");
    
    verbose::log("Agent creation completed successfully", "Agent Creation");
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
    std::cout << "[DEBUG] submitTask: Entry - agent_id=" << agent_id << ", task=" << task_keyword << std::endl;
    std::cout.flush();
    
    // Get agent pointer (lock is released after this)
    Agent* agent = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = agents_.find(agent_id);
        if (it != agents_.end()) {
            agent = it->second.get();
        }
    }
    
    if (!agent) {
        std::cout << "[DEBUG] submitTask: ERROR - Agent not found!" << std::endl;
        std::cout.flush();
        return "Error: Agent not found: " + agent_id;
    }
    
    std::cout << "[DEBUG] submitTask: Agent found, calling processTask (lock released)" << std::endl;
    std::cout.flush();
    // Call processTask without holding the lock
    std::string result = agent->processTask(task_keyword);
    std::cout << "[DEBUG] submitTask: processTask returned " << result.length() << " chars" << std::endl;
    std::cout.flush();
    return result;
}

void AgentManager::startMessageProcessing(const std::string& agent_id) {
    std::cout << "[DEBUG] startMessageProcessing: Entry for agent " << agent_id << std::endl;
    std::cout.flush();
    verbose::log("Starting message processing for agent: " + agent_id, "Message Processing");
    
    {
        std::cout << "[DEBUG] startMessageProcessing: Acquiring lock to check thread_running" << std::endl;
        std::cout.flush();
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (thread_running_[agent_id]) {
            std::cout << "[DEBUG] startMessageProcessing: Thread already running, returning" << std::endl;
            std::cout.flush();
            verbose::log("Message processing already running for agent: " + agent_id, "Message Processing");
            return; // Already running
        }
        
        std::cout << "[DEBUG] startMessageProcessing: Setting thread_running[" << agent_id << "] = true" << std::endl;
        std::cout.flush();
        thread_running_[agent_id] = true;
    }
    std::cout << "[DEBUG] startMessageProcessing: Lock released, thread_running flag set" << std::endl;
    std::cout.flush();
    
    verbose::log("Thread running flag set, creating thread", "Message Processing");
    std::cout << "[DEBUG] startMessageProcessing: About to create std::thread" << std::endl;
    std::cout.flush();
    
    // Create thread - this is non-blocking, thread will run in background
    // Release lock before creating thread to avoid deadlock
    try {
        std::cout << "[DEBUG] startMessageProcessing: Creating thread object..." << std::endl;
        std::cout.flush();
        message_threads_[agent_id] = std::thread(&AgentManager::messageProcessingLoop, this, agent_id);
        std::cout << "[DEBUG] startMessageProcessing: Thread object created successfully" << std::endl;
        std::cout.flush();
        verbose::log("Thread created successfully", "Message Processing");
        
        std::cout << "[DEBUG] startMessageProcessing: Thread created, checking status..." << std::endl;
        std::cout.flush();
        // Reactive wait: check thread status without fixed delay
        // Use a future or condition variable to wait for thread to actually start
        // For now, just proceed - thread will start asynchronously
        std::cout << "[DEBUG] startMessageProcessing: Thread starting asynchronously, detaching..." << std::endl;
        std::cout.flush();
        
        // Detach thread so it runs independently (non-blocking)
        message_threads_[agent_id].detach();
        std::cout << "[DEBUG] startMessageProcessing: Thread detached successfully" << std::endl;
        std::cout.flush();
        verbose::log("Thread detached, running in background", "Message Processing");
        
        std::cout << "[DEBUG] startMessageProcessing: Exiting successfully" << std::endl;
        std::cout.flush();
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] startMessageProcessing: EXCEPTION: " << e.what() << std::endl;
        std::cout.flush();
        verbose::log("Error creating thread: " + std::string(e.what()), "Message Processing");
        std::lock_guard<std::mutex> lock(mutex_);
        thread_running_[agent_id] = false;
    }
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
    
    // Note: Threads are detached, so we don't need to join them
    // The queue close will wake up the thread and it will exit
    // Clean up the thread entry if it exists (though it may already be cleaned up)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        message_threads_.erase(agent_id);
    }
    verbose::log("Message processing stopped for agent: " + agent_id, "Message Processing");
}

void AgentManager::stopAllMessageProcessing() {
    std::vector<std::string> agent_ids = getAgentIds();
    for (const auto& agent_id : agent_ids) {
        stopMessageProcessing(agent_id);
    }
}

void AgentManager::messageProcessingLoop(const std::string& agent_id) {
    // DEBUG: Thread entry point - this confirms the thread started
    std::cout << "[DEBUG] messageProcessingLoop: THREAD STARTED for agent " << agent_id << std::endl;
    std::cout.flush();
    
    // Log immediately when thread starts - this helps debug thread startup
    verbose::logPhase("MESSAGE PROCESSING LOOP: " + agent_id);
    verbose::logStep("Message Loop", 1, "Looking up agent and queue");
    
    std::cout << "[DEBUG] messageProcessingLoop: About to acquire lock to look up agent/queue" << std::endl;
    std::cout.flush();
    
    // Get agent and queue - need to access protected members
    Agent* agent = nullptr;
    communication::MessageQueue* queue = nullptr;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto agent_it = agents_.find(agent_id);
        if (agent_it == agents_.end()) {
            verbose::log("Agent not found - exiting", "Message Loop");
            thread_running_[agent_id] = false;
            return;
        }
        agent = agent_it->second.get();
        
        auto queue_it = agent_queues_.find(agent_id);
        if (queue_it == agent_queues_.end()) {
            verbose::log("Queue not found - exiting", "Message Loop");
            thread_running_[agent_id] = false;
            return;
        }
        queue = queue_it->second.get();
    }
    verbose::log("Agent and queue found, entering message loop", "Message Loop");
    std::cout << "[DEBUG] messageProcessingLoop: Agent and queue found, entering main loop" << std::endl;
    std::cout.flush();
    
    communication::Message message;
    
    std::cout << "[DEBUG] messageProcessingLoop: Starting main while loop" << std::endl;
    std::cout.flush();
    
    // Loop until thread is stopped
    while (true) {
        std::cout << "[DEBUG] messageProcessingLoop: Loop iteration start" << std::endl;
        std::cout.flush();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!thread_running_[agent_id]) {
                verbose::log("Thread running flag cleared - exiting loop", "Message Loop");
                break;
            }
        }
        
        // This will block waiting for messages - that's expected behavior
        // Don't log every wait cycle to avoid spam
        if (queue->pop(message)) {
            verbose::log("Message received: type=" + std::to_string(static_cast<int>(message.type)) + 
                        ", from=" + message.from_agent_id, "Message Loop");
            verbose::log("Processing message", "Message Loop");
            agent->handleMessage(message);
            verbose::log("Message processed", "Message Loop");
        } else {
            verbose::log("Queue was closed - exiting loop", "Message Loop");
            // Queue was closed
            break;
        }
    }
    verbose::log("Message processing loop exited", "Message Loop");
}

bool AgentManager::initializeAgent(const std::string& agent_id) {
    Agent* agent = getAgent(agent_id);
    if (!agent) {
        return false;
    }
    
    return agent->initialize();
}

} // namespace agent

