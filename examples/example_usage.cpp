// Example usage of the Multi-Agent LLM System C++ API

#include "../src/agent_manager.hpp"
#include "../src/task_interface.hpp"
#include "../src/reporting.hpp"
#include <iostream>
#include <memory>

int main() {
    // Create agent manager
    agent::AgentManager manager;
    
    // Create agents programmatically
    agent::AgentConfig config1;
    config1.id = "research_agent";
    config1.model_path = "models/research_model.bin";
    config1.trace_limit = 30;
    config1.memory_size = 8192;
    
    if (!manager.createAgent(config1)) {
        std::cerr << "Failed to create research_agent" << std::endl;
        return 1;
    }
    
    agent::AgentConfig config2;
    config2.id = "analysis_agent";
    config2.model_path = "models/analysis_model.bin";
    config2.trace_limit = 25;
    config2.memory_size = 6144;
    
    if (!manager.createAgent(config2)) {
        std::cerr << "Failed to create analysis_agent" << std::endl;
        return 1;
    }
    
    // Create task API
    task_interface::TaskAPI task_api(&manager);
    
    // Set up reporting with callback
    auto callback_reporter = std::make_unique<reporting::CallbackReporter>(
        [](const reporting::Report& report) {
            std::cout << "Callback received report from " << report.agent_id << std::endl;
            std::cout << "Task: " << report.task_keyword << std::endl;
            std::cout << "Findings: " << report.findings.substr(0, 100) << "..." << std::endl;
        }
    );
    
    // Submit tasks
    std::cout << "Submitting task to research_agent..." << std::endl;
    std::string result1 = task_api.submitTask("research quantum computing", "research_agent");
    std::cout << "Result: " << result1.substr(0, 200) << "..." << std::endl;
    
    std::cout << "\nSubmitting task to analysis_agent..." << std::endl;
    std::string result2 = task_api.submitTask("analyze data patterns", "analysis_agent");
    std::cout << "Result: " << result2.substr(0, 200) << "..." << std::endl;
    
    // Demonstrate inter-agent communication
    agent::Agent* agent1 = manager.getAgent("research_agent");
    if (agent1) {
        agent1->sendMessage("analysis_agent", 
                           communication::MessageType::FINDINGS_SHARING,
                           "Found interesting patterns in quantum algorithms");
    }
    
    // Cleanup
    manager.stopAllMessageProcessing();
    
    return 0;
}

