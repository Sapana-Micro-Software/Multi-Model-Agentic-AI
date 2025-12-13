#ifndef TASK_INTERFACE_HPP
#define TASK_INTERFACE_HPP

#include "agent_manager.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace task_interface {

/**
 * Task structure
 */
struct Task {
    std::string keyword;
    std::string agent_id;
    std::map<std::string, std::string> parameters;
    
    Task() = default;
    Task(const std::string& kw, const std::string& agent)
        : keyword(kw)
        , agent_id(agent)
    {}
};

/**
 * Command-line argument parser
 */
class CLIParser {
public:
    CLIParser(int argc, char* argv[]);
    
    /**
     * Parse command-line arguments
     * @return true if parsing successful
     */
    bool parse();
    
    /**
     * Get task from parsed arguments
     */
    Task getTask() const { return task_; }
    
    /**
     * Get config file path
     */
    std::string getConfigPath() const { return config_path_; }
    
    /**
     * Check if help was requested
     */
    bool isHelpRequested() const { return help_requested_; }
    
    /**
     * Check if verbose mode is enabled
     */
    bool isVerbose() const { return verbose_; }
    
    /**
     * Print usage information
     */
    void printUsage() const;

private:
    int argc_;
    char** argv_;
    Task task_;
    std::string config_path_;
    bool help_requested_;
    bool verbose_;
    
    /**
     * Parse argument
     */
    bool parseArgument(const std::string& arg, const std::string& value);
};

/**
 * Configuration file parser (YAML/JSON)
 */
class ConfigParser {
public:
    ConfigParser() = default;
    
    /**
     * Load configuration from file
     * @param file_path Path to config file
     * @return true if successful
     */
    bool loadConfig(const std::string& file_path);
    
    /**
     * Get agent configurations
     * @return Vector of agent configurations
     */
    std::vector<agent::AgentConfig> getAgentConfigs() const { return agent_configs_; }
    
    /**
     * Get default task (if specified in config)
     */
    Task getDefaultTask() const { return default_task_; }

private:
    std::vector<agent::AgentConfig> agent_configs_;
    Task default_task_;
    
    /**
     * Parse YAML config file
     */
    bool parseYAML(const std::string& file_path);
    
    /**
     * Parse JSON config file
     */
    bool parseJSON(const std::string& file_path);
    
    /**
     * Simple YAML parser (basic implementation)
     */
    bool parseSimpleYAML(const std::string& content);
};

/**
 * C++ API for task submission
 */
class TaskAPI {
public:
    TaskAPI(agent::AgentManager* manager);
    
    /**
     * Submit a task programmatically
     * @param task Task to submit
     * @return Task result
     */
    std::string submitTask(const Task& task);
    
    /**
     * Submit a task with keyword and agent ID
     * @param keyword Task keyword
     * @param agent_id Agent ID
     * @return Task result
     */
    std::string submitTask(const std::string& keyword, const std::string& agent_id);
    
    /**
     * Create agent programmatically
     * @param config Agent configuration
     * @return true if successful
     */
    bool createAgent(const agent::AgentConfig& config);
    
    /**
     * Get agent manager
     */
    agent::AgentManager* getAgentManager() const { return agent_manager_; }

private:
    agent::AgentManager* agent_manager_;
};

} // namespace task_interface

#endif // TASK_INTERFACE_HPP

