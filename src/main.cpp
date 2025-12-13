/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 */

#include "agent_manager.hpp"
#include "task_interface.hpp"
#include "reporting.hpp"
#include "security/security.hpp"
#include "fault_tolerance/retry.hpp"
#include "verbose.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>

/**
 * Display internal state of the multi-agent system (verbose mode)
 */
void displayInternalState(agent::AgentManager& agent_manager, bool verbose) {
    if (!verbose) return;
    
    std::cout << "\n" << std::string(80, '=') << "\n";
    std::cout << "INTERNAL STATE - MULTI-AGENT LLM SYSTEM\n";
    std::cout << std::string(80, '=') << "\n\n";
    
    auto agent_ids = agent_manager.getAgentIds();
    std::cout << "Total Agents: " << agent_ids.size() << "\n\n";
    
    for (const auto& agent_id : agent_ids) {
        agent::Agent* agent = agent_manager.getAgent(agent_id);
        if (!agent) continue;
        
        std::cout << std::string(80, '-') << "\n";
        std::cout << "AGENT: " << agent_id << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        // Agent status
        std::cout << "Status: " << (agent->isInitialized() ? "✓ Initialized" : "✗ Not Initialized") << "\n";
        
        // Memory traces
        auto trace_history = agent->getTraceHistory();
        std::cout << "Trace History Entries: " << trace_history.size() << "\n";
        
        if (!trace_history.empty()) {
            std::cout << "\nRecent Traces (showing last " << std::min(trace_history.size(), size_t(3)) << "):\n";
            size_t display_count = std::min(trace_history.size(), size_t(3));
            for (size_t i = trace_history.size() - display_count; i < trace_history.size(); ++i) {
                const auto& trace = trace_history[i];
                std::cout << "  [" << (i + 1) << "] Task: \"" << trace.task_keyword << "\"\n";
                std::cout << "      Reasoning Steps: " << trace.reasoning_steps.size() << "\n";
                if (!trace.reasoning_steps.empty() && verbose) {
                    std::cout << "      First Step: ";
                    std::string first_step = trace.reasoning_steps[0];
                    if (first_step.length() > 50) {
                        first_step = first_step.substr(0, 47) + "...";
                    }
                    std::cout << first_step << "\n";
                }
                std::cout << "      Key Insights: " << trace.key_insights.size() << "\n";
                if (!trace.key_insights.empty() && verbose) {
                    std::cout << "      Sample Insight: " << trace.key_insights[0] << "\n";
                }
                if (!trace.summary.empty()) {
                    std::string summary_preview = trace.summary.substr(0, 80);
                    if (trace.summary.length() > 80) summary_preview += "...";
                    std::cout << "      Summary: " << summary_preview << "\n";
                }
                if (!trace.findings.empty()) {
                    std::string findings_preview = trace.findings.substr(0, 80);
                    if (trace.findings.length() > 80) findings_preview += "...";
                    std::cout << "      Findings: " << findings_preview << "\n";
                }
            }
        }
        
        // World model preview
        std::string world_model = agent->getWorldModel();
        if (!world_model.empty() && world_model != "Initial world model state") {
            std::cout << "\nWorld Model:\n";
            std::string preview = world_model.substr(0, 300);
            if (world_model.length() > 300) preview += "...";
            // Split into lines for readability
            std::istringstream wm_stream(preview);
            std::string wm_line;
            while (std::getline(wm_stream, wm_line)) {
                std::cout << "  " << wm_line << "\n";
            }
        } else {
            std::cout << "\nWorld Model: (Initial/Empty)\n";
        }
        
        // Normalized context info
        std::string context = agent->getNormalizedContext();
        std::cout << "\nNormalized Context:\n";
        std::cout << "  Size: " << context.length() << " bytes\n";
        if (!context.empty() && verbose) {
            std::string context_preview = context.substr(0, 200);
            if (context.length() > 200) context_preview += "...";
            std::cout << "  Preview: " << context_preview << "\n";
        }
        
        std::cout << "\n";
    }
    
    std::cout << std::string(80, '=') << "\n\n";
}

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
        
        bool verbose = cli_parser.isVerbose();
        verbose::enabled = verbose; // Set global verbose flag
        
        if (verbose) {
            verbose::logPhase("SYSTEM INITIALIZATION");
            verbose::log("Starting multi-agent LLM system");
        }
        
        // Create agent manager
        agent::AgentManager agent_manager;
        
        // Load configuration if provided
        std::string config_path = cli_parser.getConfigPath();
        if (config_path.empty()) {
            // Try default config path
            config_path = "config/agents.yaml";
        }
        
        // Try to resolve relative paths
        // If config_path starts with ../, it's relative to current directory
        // Otherwise, try relative to project root
        if (verbose) {
            std::cout << "[VERBOSE] Resolving config path: " << config_path << std::endl;
            std::cout.flush();
        }
        std::string resolved_path = config_path;
        if (config_path.find("../") == 0) {
            // Already relative, use as-is
        } else if (config_path.find("/") != 0) {
            // Relative path, try from project root
            std::ifstream test_file(config_path);
            if (!test_file.good()) {
                // Try with ../ prefix (if running from build/)
                resolved_path = "../" + config_path;
            }
            test_file.close();
        }
        
        if (verbose) {
            std::cout << "[VERBOSE] Loading config from: " << resolved_path << std::endl;
            std::cout.flush();
        }
        task_interface::ConfigParser config_parser;
        bool config_result = config_parser.loadConfig(resolved_path);
        if (verbose) {
            std::cout << "[VERBOSE] Config load returned: " << (config_result ? "true" : "false") << std::endl;
            std::cout.flush();
        }
        if (config_result) {
            if (verbose) {
                std::cout << "[VERBOSE] Config loaded successfully, getting agent configs..." << std::endl;
                std::cout.flush();
            }
            // Create agents from config
            auto agent_configs = config_parser.getAgentConfigs();
            if (verbose) {
                std::cout << "[VERBOSE] Retrieved " << agent_configs.size() << " agent config(s)" << std::endl;
                std::cout.flush();
            }
            for (size_t i = 0; i < agent_configs.size(); ++i) {
                const auto& config = agent_configs[i];
                if (verbose) {
                    std::cout << "[VERBOSE] Creating agent: " << config.id << " with model: " << config.model_path << std::endl;
                    std::cout.flush();
                }
                if (!agent_manager.createAgent(config)) {
                    std::cerr << "Warning: Failed to create agent: " << config.id << std::endl;
                    std::cerr.flush();
                } else {
                    if (verbose) {
                        std::cout << "[VERBOSE] Agent created successfully: " << config.id 
                                 << " (model: " << config.model_path 
                                 << ", trace_limit: " << config.trace_limit
                                 << ", memory_size: " << config.memory_size << ")" << std::endl;
                        std::cout.flush();
                    } else {
                        std::cout << "Created agent: " << config.id << std::endl;
                    }
                }
            }
        } else {
            if (verbose) {
                std::cerr << "[VERBOSE] Warning: Could not load config file: " << config_path << std::endl;
                std::cerr.flush();
            } else {
                std::cerr << "Warning: Could not load config file: " << config_path << std::endl;
            }
            std::cerr << "Continuing without pre-configured agents..." << std::endl;
            std::cerr.flush();
        }
        
        if (verbose) {
            std::cout << "[VERBOSE] Agents loaded: " << agent_manager.getAgentCount() << std::endl;
            std::cout.flush();
        }
        
        // Display initial state if verbose
        if (verbose && agent_manager.getAgentCount() > 0) {
            std::cout << "[VERBOSE] Displaying initial agent state..." << std::endl;
            std::cout.flush();
            displayInternalState(agent_manager, verbose);
            std::cout.flush();
        }
        
        if (verbose) {
            std::cout << "[VERBOSE] Proceeding to task processing..." << std::endl;
            std::cout.flush();
        }
        
        verbose::logPhase("TASK SETUP");
        verbose::logStep("Task Setup", 1, "Initializing input validator and retry executor");
        // Get task from CLI with input validation and recursive retry
        security::InputValidator validator(3); // Max 3 retries
        fault_tolerance::RetryExecutor<std::string> retry_executor;
        
        verbose::logStep("Task Setup", 2, "Getting task from CLI");
        task_interface::Task task = cli_parser.getTask();
        
        // If no task specified, check config for default task
        if (task.keyword.empty()) {
            verbose::logStep("Task Setup", 3, "No task from CLI, checking config for default task");
            task = config_parser.getDefaultTask();
        }
        
        // If still no task, prompt user or exit
        if (task.keyword.empty()) {
            verbose::log("No task specified - error", "Task Setup");
            std::cerr << "Error: No task specified. Use --task to specify a task keyword.\n";
            cli_parser.printUsage();
            return 1;
        }
        
        verbose::log("Task keyword: " + task.keyword, "Task Setup");
        verbose::logStep("Task Setup", 4, "Validating and sanitizing task keyword");
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
                
                if (verbose) {
                    std::cout << "[VERBOSE] No agents configured. Creating default agent..." << std::endl;
                } else {
                    std::cout << "No agents configured. Creating default agent..." << std::endl;
                }
                if (!agent_manager.createAgent(default_agent_id, default_model_path)) {
                    std::cerr << "Error: Failed to create default agent. "
                              << "Please provide a valid model path in config or create agent manually.\n";
                    return 1;
                }
                if (verbose) {
                    std::cout << "[VERBOSE] Default agent created successfully: " << default_agent_id << std::endl;
                    std::cout.flush();
                    displayInternalState(agent_manager, verbose);
                    std::cout.flush();
                }
                task.agent_id = default_agent_id;
            } else {
                task.agent_id = agent_ids[0];
                std::cout << "No agent specified. Using first available agent: " 
                         << task.agent_id << std::endl;
            }
        }
        
        verbose::log("Task agent ID: " + (task.agent_id.empty() ? "<none>" : task.agent_id), "Task Setup");
        verbose::logStep("Task Setup", 5, "Verifying agent exists");
        // Verify agent exists
        if (!agent_manager.hasAgent(task.agent_id)) {
            verbose::log("Agent not found: " + task.agent_id, "Task Setup");
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
        
        verbose::logPhase("REPORTING SETUP");
        verbose::logStep("Reporting Setup", 1, "Creating console and file reporters");
        // Set up reporting
        auto console_reporter = std::make_unique<reporting::ConsoleReporter>(false);
        auto file_reporter = std::make_unique<reporting::FileReporter>("reports", true);
        auto composite_reporter = std::make_unique<reporting::CompositeReporter>();
        
        composite_reporter->addReporter(std::move(console_reporter));
        composite_reporter->addReporter(std::move(file_reporter));
        verbose::log("Reporters created and added to composite", "Reporting Setup");
        
        verbose::logPhase("TASK EXECUTION");
        std::cout << "[DEBUG MAIN] About to create TaskAPI" << std::endl;
        std::cout.flush();
        verbose::logStep("Task Execution", 1, "Creating task API");
        // Create task API
        task_interface::TaskAPI task_api(&agent_manager);
        
        std::cout << "[DEBUG MAIN] TaskAPI created, about to submit task" << std::endl;
        std::cout << "[DEBUG MAIN] Task: \"" << task.keyword << "\" to agent: \"" << task.agent_id << "\"" << std::endl;
        std::cout.flush();
        verbose::logStep("Task Execution", 2, "Submitting task: \"" + task.keyword + "\" to agent: \"" + task.agent_id + "\"");
        // Submit task with retry mechanism
        if (verbose) {
            std::cout << "[VERBOSE] Submitting task \"" << task.keyword 
                     << "\" to agent \"" << task.agent_id << "\"..." << std::endl;
            std::cout.flush();
        } else {
            std::cout << "Submitting task \"" << task.keyword 
                     << "\" to agent \"" << task.agent_id << "\"..." << std::endl;
        }
        
        verbose::logStep("Task Execution", 3, "Configuring retry policy");
        fault_tolerance::RetryPolicy policy;
        policy.max_attempts = 3;
        policy.initial_delay = std::chrono::milliseconds(100);
        policy.exponential_backoff = true;
        
        verbose::logStep("Task Execution", 4, "Creating retry executor");
        fault_tolerance::RetryExecutor<std::string> task_retry(policy);
        
        verbose::logStep("Task Execution", 5, "Executing task with retry mechanism");
        std::string result = task_retry.execute([&task_api, &task, &verbose]() {
            verbose::log("Task execution attempt", "Task Execution");
            if (verbose) {
                std::cout << "[VERBOSE] Executing task..." << std::endl;
                std::cout.flush();
            }
            return task_api.submitTask(task);
        }, [](const std::string& res) {
            // Retry if result indicates error
            bool should_retry = res.find("Error:") != std::string::npos;
            if (should_retry) {
                verbose::log("Task result indicates error - will retry", "Task Execution");
            }
            return should_retry;
        });
        
        verbose::log("Task execution completed", "Task Execution");
        verbose::log("Result length: " + std::to_string(result.length()) + " bytes", "Task Execution");
        if (verbose) {
            std::cout << "[VERBOSE] Task execution completed. Result length: " << result.length() << " bytes" << std::endl;
            std::cout.flush();
        }
        
        verbose::logPhase("RESULT REPORTING");
        verbose::logStep("Reporting", 1, "Creating report structure");
        // Create report from result
        reporting::Report report;
        report.agent_id = task.agent_id;
        report.task_keyword = task.keyword;
        report.findings = result;
        
        verbose::logStep("Reporting", 2, "Extracting agent details for report");
        // Get agent to extract more details
        agent::Agent* agent = agent_manager.getAgent(task.agent_id);
        if (agent) {
            verbose::log("Agent found, extracting trace history", "Reporting");
            auto trace_history = agent->getTraceHistory();
            if (!trace_history.empty()) {
                const auto& latest_trace = trace_history.back();
                report.reasoning_steps = latest_trace.reasoning_steps;
                report.key_insights = latest_trace.key_insights;
                verbose::log("Extracted " + std::to_string(report.reasoning_steps.size()) + 
                            " reasoning steps and " + std::to_string(report.key_insights.size()) + 
                            " key insights", "Reporting");
            }
        }
        
        verbose::logStep("Reporting", 3, "Sending report to composite reporter");
        // Report results
        composite_reporter->report(report);
        composite_reporter->flush();
        verbose::log("Report sent and flushed", "Reporting");
        
        std::cout << "\nTask completed successfully." << std::endl;
        
        // Display final state if verbose
        if (verbose) {
            displayInternalState(agent_manager, verbose);
        }
        
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

