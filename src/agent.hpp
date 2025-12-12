#ifndef AGENT_HPP
#define AGENT_HPP

#include "llm_wrapper.hpp"
#include "memory.hpp"
#include "communication.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>

namespace agent {

// Forward declaration
class AgentManager;

/**
 * Agent class representing an individual agent with LLM, memory, and reasoning capabilities
 */
class Agent {
public:
    /**
     * Constructor
     * @param agent_id Unique identifier for the agent
     * @param model_path Path to the LLM model file
     * @param trace_limit Maximum number of traces to keep
     * @param memory_size Maximum memory size in bytes
     */
    Agent(const std::string& agent_id,
          const std::string& model_path,
          size_t trace_limit = 20,
          size_t memory_size = 4096);
    
    ~Agent();
    
    // Disable copy constructor and assignment
    Agent(const Agent&) = delete;
    Agent& operator=(const Agent&) = delete;
    
    /**
     * Get agent ID
     */
    const std::string& getId() const { return agent_id_; }
    
    /**
     * Initialize the agent (load model, etc.)
     * @return true if successful
     */
    bool initialize();
    
    /**
     * Check if agent is initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * Process a task given by a keyword
     * @param task_keyword Keyword describing the task
     * @return Findings report
     */
    std::string processTask(const std::string& task_keyword);
    
    /**
     * Ruminate on a task using chain-of-thought reasoning
     * @param task_keyword Task keyword
     * @return Structured reasoning and findings
     */
    memory::TraceEntry ruminate(const std::string& task_keyword);
    
    /**
     * Update world model with new information
     * @param information New information to incorporate
     */
    void updateWorldModel(const std::string& information);
    
    /**
     * Get current world model state
     */
    std::string getWorldModel() const { return world_model_; }
    
    /**
     * Handle incoming message from another agent
     * @param message Incoming message
     */
    void handleMessage(const communication::Message& message);
    
    /**
     * Send message to another agent
     * @param to_agent_id Target agent ID
     * @param message_type Type of message
     * @param payload Message payload
     */
    void sendMessage(const std::string& to_agent_id,
                    communication::MessageType message_type,
                    const std::string& payload);
    
    /**
     * Set agent manager reference (for message routing)
     */
    void setAgentManager(AgentManager* manager) { agent_manager_ = manager; }
    
    /**
     * Get trace history
     */
    std::vector<memory::TraceEntry> getTraceHistory() const;
    
    /**
     * Get normalized context for LLM
     */
    std::string getNormalizedContext() const;

private:
    std::string agent_id_;
    std::string model_path_;
    std::string world_model_;
    
    std::unique_ptr<llm_wrapper::LLMWrapper> llm_;
    std::unique_ptr<memory::TraceManager> trace_manager_;
    
    bool initialized_;
    AgentManager* agent_manager_;
    
    /**
     * Generate chain-of-thought reasoning steps
     * @param task_keyword Task to reason about
     * @return Vector of reasoning steps
     */
    std::vector<std::string> generateReasoningSteps(const std::string& task_keyword);
    
    /**
     * Reflect on a reasoning step
     * @param step Reasoning step to reflect on
     * @return Reflection result
     */
    std::string reflectOnStep(const std::string& step);
    
    /**
     * Synthesize findings from reasoning steps
     * @param reasoning_steps Vector of reasoning steps
     * @return Synthesized findings
     */
    std::string synthesizeFindings(const std::vector<std::string>& reasoning_steps);
    
    /**
     * Extract key insights from findings
     * @param findings Findings text
     * @return Vector of key insights
     */
    std::vector<std::string> extractKeyInsights(const std::string& findings);
    
    /**
     * Generate summary of trace entry
     * @param entry Trace entry
     * @return Summary string
     */
    std::string generateSummary(const memory::TraceEntry& entry);
    
    /**
     * Build prompt for LLM with context
     * @param task_keyword Task keyword
     * @return Full prompt with context
     */
    std::string buildPrompt(const std::string& task_keyword);
};

} // namespace agent

#endif // AGENT_HPP

