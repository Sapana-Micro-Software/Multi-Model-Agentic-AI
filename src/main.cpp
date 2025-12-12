/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * No MIT License
 */

#include "agent_manager.hpp"
#include "task_interface.hpp"
#include "reporting.hpp"
#include "security/security.hpp"
#include "fault_tolerance/retry.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

int main(int argc, char* argv[]) {
    try {
        // Parse command-line arguments
        task_interface::CLIParser cli_parser(argc, argv);
        if (!cli_parser.parse()) {
            if (!cli_parser.isHelpRequested()) {
                std::cerr << "Error: Failed to parse command-line arguments\n";
                cli_parser.printUsage();
                return 1;
            }
            cli_parser.printUsage();
            return 0;
        }
        
        if (cli_parser.isHelpRequested()) {
            cli_parser.printUsage();
            return 0;
        }
        
        // Create agent manager
        agent::AgentManager agent_manager;
        
        // Load configuration if provided
        std::string config_path = cli_parser.getConfigPath();
        if (config_path.empty()) {
            // Try default config path
            config_path = "config/agents.yaml";
        }
        
        task_interface::ConfigParser config_parser;
        if (config_parser.loadConfig(config_path)) {
            // Create agents from config
            auto agent_configs = config_parser.getAgentConfigs();
            for (const auto& config : agent_configs) {
                if (!agent_manager.createAgent(config)) {
                    std::cerr << "Warning: Failed to create agent: " << config.id << std::endl;
                } else {
                    std::cout << "Created agent: " << config.id << std::endl;
                }
            }
        } else {
            std::cerr << "Warning: Could not load config file: " << config_path << std::endl;
            std::cerr << "Continuing without pre-configured agents..." << std::endl;
        }
        
        // Get task from CLI with input validation and recursive retry
        security::InputValidator validator(3); // Max 3 retries
        fault_tolerance::RetryExecutor<std::string> retry_executor;
        
        task_interface::Task task = cli_parser.getTask();
        
        // If no task specified, check config for default task
        if (task.keyword.empty()) {
            task = config_parser.getDefaultTask();
        }
        
        // If still no task, prompt user or exit
        if (task.keyword.empty()) {
            std::cerr << "Error: No task specified. Use --task to specify a task keyword.\n";
            cli_parser.printUsage();
            return 1;
        }
        
        // Validate and sanitize task keyword with recursive retry
        std::string validated_keyword = validator.validateWithRetry(
            task.keyword,
            [&validator](const std::string& kw) {
                return validator.validateTaskKeyword(kw);
            },
            [&validator](const std::string& kw) {
                return validator.sanitize(kw);
            }
        );
        
        if (validated_keyword.empty()) {
            std::cerr << "Error: Task keyword validation failed after retries.\n";
            return 1;
        }
        
        task.keyword = validated_keyword;
        
        // Validate agent ID if provided
        if (!task.agent_id.empty()) {
            std::string validated_agent_id = validator.validateWithRetry(
                task.agent_id,
                [&validator](const std::string& id) {
                    return validator.validateAgentId(id);
                },
                [&validator](const std::string& id) {
                    return validator.sanitize(id);
                }
            );
            
            if (validated_agent_id.empty()) {
                std::cerr << "Warning: Agent ID validation failed. Continuing with sanitized version.\n";
            } else {
                task.agent_id = validated_agent_id;
            }
        }
        
        // If no agent specified, use first available agent or create default
        if (task.agent_id.empty()) {
            auto agent_ids = agent_manager.getAgentIds();
            if (agent_ids.empty()) {
                // Create a default agent
                std::string default_agent_id = "default_agent";
                std::string default_model_path = "models/default.bin";
                
                std::cout << "No agents configured. Creating default agent..." << std::endl;
                if (!agent_manager.createAgent(default_agent_id, default_model_path)) {
                    std::cerr << "Error: Failed to create default agent. "
                              << "Please provide a valid model path in config or create agent manually.\n";
                    return 1;
                }
                task.agent_id = default_agent_id;
            } else {
                task.agent_id = agent_ids[0];
                std::cout << "No agent specified. Using first available agent: " 
                         << task.agent_id << std::endl;
            }
        }
        
        // Verify agent exists
        if (!agent_manager.hasAgent(task.agent_id)) {
            std::cerr << "Error: Agent not found: " << task.agent_id << std::endl;
            std::cerr << "Available agents: ";
            auto agent_ids = agent_manager.getAgentIds();
            for (size_t i = 0; i < agent_ids.size(); ++i) {
                std::cerr << agent_ids[i];
                if (i < agent_ids.size() - 1) {
                    std::cerr << ", ";
                }
            }
            std::cerr << std::endl;
            return 1;
        }
        
        // Set up reporting
        auto console_reporter = std::make_unique<reporting::ConsoleReporter>(false);
        auto file_reporter = std::make_unique<reporting::FileReporter>("reports", true);
        auto composite_reporter = std::make_unique<reporting::CompositeReporter>();
        
        composite_reporter->addReporter(std::move(console_reporter));
        composite_reporter->addReporter(std::move(file_reporter));
        
        // Create task API
        task_interface::TaskAPI task_api(&agent_manager);
        
        // Submit task with retry mechanism
        std::cout << "Submitting task \"" << task.keyword 
                 << "\" to agent \"" << task.agent_id << "\"..." << std::endl;
        
        fault_tolerance::RetryPolicy policy;
        policy.max_attempts = 3;
        policy.initial_delay = std::chrono::milliseconds(100);
        policy.exponential_backoff = true;
        
        fault_tolerance::RetryExecutor<std::string> task_retry(policy);
        
        std::string result = task_retry.execute([&task_api, &task]() {
            return task_api.submitTask(task);
        }, [](const std::string& res) {
            // Retry if result indicates error
            return res.find("Error:") != std::string::npos;
        });
        
        // Create report from result
        reporting::Report report;
        report.agent_id = task.agent_id;
        report.task_keyword = task.keyword;
        report.findings = result;
        
        // Get agent to extract more details
        agent::Agent* agent = agent_manager.getAgent(task.agent_id);
        if (agent) {
            auto trace_history = agent->getTraceHistory();
            if (!trace_history.empty()) {
                const auto& latest_trace = trace_history.back();
                report.reasoning_steps = latest_trace.reasoning_steps;
                report.key_insights = latest_trace.key_insights;
            }
        }
        
        // Report results
        composite_reporter->report(report);
        composite_reporter->flush();
        
        std::cout << "\nTask completed successfully." << std::endl;
        
        // Cleanup
        agent_manager.stopAllMessageProcessing();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
        return 1;
    }
}

