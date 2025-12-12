#ifndef AGENT_MANAGER_HPP
#define AGENT_MANAGER_HPP

#include "agent.hpp"
#include "communication.hpp"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>

namespace agent {

/**
 * Agent configuration structure
 */
struct AgentConfig {
    std::string id;
    std::string model_path;
    size_t trace_limit;
    size_t memory_size;
    
    AgentConfig()
        : trace_limit(20)
        , memory_size(4096)
    {}
};

/**
 * AgentManager class for creating and managing multiple agents
 */
class AgentManager {
public:
    AgentManager();
    ~AgentManager();
    
    // Disable copy
    AgentManager(const AgentManager&) = delete;
    AgentManager& operator=(const AgentManager&) = delete;
    
    /**
     * Create an agent from configuration
     * @param config Agent configuration
     * @return true if successful
     */
    bool createAgent(const AgentConfig& config);
    
    /**
     * Create an agent dynamically
     * @param agent_id Unique agent ID
     * @param model_path Path to model file
     * @param trace_limit Trace limit
     * @param memory_size Memory size
     * @return true if successful
     */
    bool createAgent(const std::string& agent_id,
                    const std::string& model_path,
                    size_t trace_limit = 20,
                    size_t memory_size = 4096);
    
    /**
     * Remove an agent
     * @param agent_id Agent ID to remove
     * @return true if agent was found and removed
     */
    bool removeAgent(const std::string& agent_id);
    
    /**
     * Get an agent by ID
     * @param agent_id Agent ID
     * @return Pointer to agent, or nullptr if not found
     */
    Agent* getAgent(const std::string& agent_id);
    
    /**
     * Check if agent exists
     * @param agent_id Agent ID
     * @return true if agent exists
     */
    bool hasAgent(const std::string& agent_id) const;
    
    /**
     * Get all agent IDs
     * @return Vector of agent IDs
     */
    std::vector<std::string> getAgentIds() const;
    
    /**
     * Get number of agents
     */
    size_t getAgentCount() const;
    
    /**
     * Route a message to target agent
     * @param message Message to route
     * @return true if routed successfully
     */
    bool routeMessage(const communication::Message& message);
    
    /**
     * Submit a task to an agent
     * @param agent_id Target agent ID
     * @param task_keyword Task keyword
     * @return Task result (findings report)
     */
    std::string submitTask(const std::string& agent_id, const std::string& task_keyword);
    
    /**
     * Start message processing thread for an agent
     * @param agent_id Agent ID
     */
    void startMessageProcessing(const std::string& agent_id);
    
    /**
     * Stop message processing for an agent
     * @param agent_id Agent ID
     */
    void stopMessageProcessing(const std::string& agent_id);
    
    /**
     * Stop all message processing threads
     */
    void stopAllMessageProcessing();

private:
    std::map<std::string, std::unique_ptr<Agent>> agents_;
    std::map<std::string, std::unique_ptr<communication::MessageQueue>> agent_queues_;
    std::map<std::string, std::thread> message_threads_;
    std::map<std::string, bool> thread_running_;
    
    communication::MessageRouter router_;
    mutable std::mutex mutex_;
    
    /**
     * Message processing loop for an agent
     * @param agent_id Agent ID
     */
    void messageProcessingLoop(const std::string& agent_id);
    
    /**
     * Initialize agent and register with router
     * @param agent_id Agent ID
     * @return true if successful
     */
    bool initializeAgent(const std::string& agent_id);
};

} // namespace agent

#endif // AGENT_MANAGER_HPP

