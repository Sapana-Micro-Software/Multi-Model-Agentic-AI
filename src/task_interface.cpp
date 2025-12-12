#include "task_interface.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

namespace task_interface {

// CLIParser Implementation

CLIParser::CLIParser(int argc, char* argv[])
    : argc_(argc)
    , argv_(argv)
    , help_requested_(false)
{
}

bool CLIParser::parse() {
    if (argc_ < 2) {
        return false;
    }
    
    for (int i = 1; i < argc_; ++i) {
        std::string arg = argv_[i];
        
        if (arg == "--help" || arg == "-h") {
            help_requested_ = true;
            return true;
        }
        
        if (arg == "--task" || arg == "-t") {
            if (i + 1 < argc_) {
                task_.keyword = argv_[++i];
            } else {
                return false;
            }
        } else if (arg == "--agent" || arg == "-a") {
            if (i + 1 < argc_) {
                task_.agent_id = argv_[++i];
            } else {
                return false;
            }
        } else if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc_) {
                config_path_ = argv_[++i];
            } else {
                return false;
            }
        } else if (arg.find("--") == 0) {
            // Parameter: --key=value or --key value
            std::string key = arg.substr(2);
            std::string value;
            
            size_t eq_pos = key.find('=');
            if (eq_pos != std::string::npos) {
                value = key.substr(eq_pos + 1);
                key = key.substr(0, eq_pos);
            } else if (i + 1 < argc_) {
                value = argv_[++i];
            }
            
            if (!key.empty() && !value.empty()) {
                task_.parameters[key] = value;
            }
        }
    }
    
    return true;
}

void CLIParser::printUsage() const {
    std::cout << "Usage: multi_agent_llm [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t, --task KEYWORD       Task keyword to process\n";
    std::cout << "  -a, --agent AGENT_ID     Agent ID to assign task to\n";
    std::cout << "  -c, --config FILE        Configuration file path\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "\n";
    std::cout << "Examples:\n";
    std::cout << "  ./multi_agent_llm --task \"research topic\" --agent agent1\n";
    std::cout << "  ./multi_agent_llm --config config/agents.yaml --task \"analyze data\"\n";
}

// ConfigParser Implementation

bool ConfigParser::loadConfig(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Determine file type by extension
    std::string ext = file_path.substr(file_path.find_last_of('.'));
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".yaml" || ext == ".yml") {
        return parseYAML(file_path);
    } else if (ext == ".json") {
        return parseJSON(file_path);
    } else {
        // Try to parse as YAML by default
        return parseYAML(file_path);
    }
}

bool ConfigParser::parseYAML(const std::string& file_path) {
    return parseSimpleYAML(file_path);
}

bool ConfigParser::parseSimpleYAML(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    agent::AgentConfig current_config;
    bool in_agents = false;
    bool in_agent = false;
    
    while (std::getline(file, line)) {
        // Remove comments
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) {
            line = line.substr(0, comment_pos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) continue;
        
        // Check for agents section
        if (line == "agents:" || line.find("agents:") == 0) {
            in_agents = true;
            continue;
        }
        
        if (in_agents) {
            // Check for agent entry
            if (line == "-" || line.find("- ") == 0) {
                if (in_agent && !current_config.id.empty()) {
                    agent_configs_.push_back(current_config);
                    current_config = agent::AgentConfig();
                }
                in_agent = true;
                continue;
            }
            
            if (in_agent) {
                // Parse key-value pairs
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::string key = line.substr(0, colon_pos);
                    std::string value = line.substr(colon_pos + 1);
                    
                    // Trim
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t\""));
                    value.erase(value.find_last_not_of(" \t\"") + 1);
                    
                    if (key == "id") {
                        current_config.id = value;
                    } else if (key == "model_path") {
                        current_config.model_path = value;
                    } else if (key == "trace_limit") {
                        current_config.trace_limit = std::stoul(value);
                    } else if (key == "memory_size") {
                        current_config.memory_size = std::stoul(value);
                    }
                }
            }
        }
    }
    
    // Add last agent if exists
    if (in_agent && !current_config.id.empty()) {
        agent_configs_.push_back(current_config);
    }
    
    file.close();
    return !agent_configs_.empty();
}

bool ConfigParser::parseJSON(const std::string& file_path) {
    // Simple JSON parser (basic implementation)
    // For production, consider using a proper JSON library like nlohmann/json
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // Very basic JSON parsing for agents array
    std::regex agent_regex(R"("id"\s*:\s*"([^"]+)"[^}]*"model_path"\s*:\s*"([^"]+)"[^}]*"trace_limit"\s*:\s*(\d+)[^}]*"memory_size"\s*:\s*(\d+))");
    std::sregex_iterator iter(content.begin(), content.end(), agent_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        agent::AgentConfig config;
        config.id = iter->str(1);
        config.model_path = iter->str(2);
        config.trace_limit = std::stoul(iter->str(3));
        config.memory_size = std::stoul(iter->str(4));
        agent_configs_.push_back(config);
    }
    
    return !agent_configs_.empty();
}

// TaskAPI Implementation

TaskAPI::TaskAPI(agent::AgentManager* manager)
    : agent_manager_(manager)
{
    if (!agent_manager_) {
        throw std::invalid_argument("AgentManager cannot be null");
    }
}

std::string TaskAPI::submitTask(const Task& task) {
    return submitTask(task.keyword, task.agent_id);
}

std::string TaskAPI::submitTask(const std::string& keyword, const std::string& agent_id) {
    if (!agent_manager_) {
        return "Error: AgentManager not initialized";
    }
    
    return agent_manager_->submitTask(agent_id, keyword);
}

bool TaskAPI::createAgent(const agent::AgentConfig& config) {
    if (!agent_manager_) {
        return false;
    }
    
    return agent_manager_->createAgent(config);
}

} // namespace task_interface

