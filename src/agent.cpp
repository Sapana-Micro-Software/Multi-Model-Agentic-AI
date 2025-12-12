#include "agent.hpp"
#include "agent_manager.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

namespace agent {

Agent::Agent(const std::string& agent_id,
             const std::string& model_path,
             size_t trace_limit,
             size_t memory_size)
    : agent_id_(agent_id)
    , model_path_(model_path)
    , world_model_("Initial world model state")
    , initialized_(false)
    , agent_manager_(nullptr)
{
    llm_ = std::make_unique<llm_wrapper::LLMWrapper>();
    trace_manager_ = std::make_unique<memory::TraceManager>(trace_limit, memory_size);
}

Agent::~Agent() = default;

bool Agent::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!llm_->loadModel(model_path_)) {
        return false;
    }
    
    initialized_ = true;
    return true;
}

std::string Agent::buildPrompt(const std::string& task_keyword) {
    std::ostringstream oss;
    
    // Add world model context
    oss << "World Model: " << world_model_ << "\n\n";
    
    // Add normalized trace context
    std::string context = trace_manager_->getNormalizedContext();
    if (!context.empty()) {
        oss << "Previous Context:\n" << context << "\n\n";
    }
    
    // Add key insights
    auto insights = trace_manager_->getKeyInsights();
    if (!insights.empty()) {
        oss << "Key Insights from History:\n";
        for (const auto& insight : insights) {
            oss << "- " << insight << "\n";
        }
        oss << "\n";
    }
    
    // Add current task
    oss << "Current Task: " << task_keyword << "\n\n";
    oss << "Please analyze this task using chain-of-thought reasoning.\n";
    
    return oss.str();
}

std::vector<std::string> Agent::generateReasoningSteps(const std::string& task_keyword) {
    if (!initialized_) {
        return {"Agent not initialized"};
    }
    
    std::string prompt = buildPrompt(task_keyword);
    prompt += "Break down the task into reasoning steps. List each step clearly.\n";
    
    std::string response = llm_->generate(prompt, 512, 0.7f, 40, 0.9f);
    
    // Parse reasoning steps from response
    std::vector<std::string> steps;
    std::regex step_regex(R"((?:^|\n)(?:\d+\.|[-*])\s*(.+?)(?=\n(?:^\d+\.|^[-*]|$)))", 
                         std::regex::multiline);
    std::sregex_iterator iter(response.begin(), response.end(), step_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string step = iter->str(1);
        // Trim whitespace
        step.erase(0, step.find_first_not_of(" \t\n\r"));
        step.erase(step.find_last_not_of(" \t\n\r") + 1);
        if (!step.empty()) {
            steps.push_back(step);
        }
    }
    
    // If no steps found, split by newlines
    if (steps.empty()) {
        std::istringstream iss(response);
        std::string line;
        while (std::getline(iss, line)) {
            if (!line.empty() && line.find_first_not_of(" \t") != std::string::npos) {
                steps.push_back(line);
            }
        }
    }
    
    // Ensure at least one step
    if (steps.empty()) {
        steps.push_back("Initial analysis of task: " + task_keyword);
    }
    
    return steps;
}

std::string Agent::reflectOnStep(const std::string& step) {
    if (!initialized_) {
        return "Agent not initialized";
    }
    
    std::ostringstream oss;
    oss << "Reflect on this reasoning step: " << step << "\n";
    oss << "Consider: Is this step valid? What are the implications? "
        << "What questions does this raise?\n";
    
    std::string reflection = llm_->generate(oss.str(), 256, 0.7f, 40, 0.9f);
    return reflection;
}

std::string Agent::synthesizeFindings(const std::vector<std::string>& reasoning_steps) {
    if (!initialized_) {
        return "Agent not initialized";
    }
    
    std::ostringstream oss;
    oss << "Based on the following reasoning steps, synthesize the key findings:\n\n";
    
    for (size_t i = 0; i < reasoning_steps.size(); ++i) {
        oss << "Step " << (i + 1) << ": " << reasoning_steps[i] << "\n";
    }
    
    oss << "\nProvide a comprehensive summary of findings.\n";
    
    std::string findings = llm_->generate(oss.str(), 512, 0.7f, 40, 0.9f);
    return findings;
}

std::vector<std::string> Agent::extractKeyInsights(const std::string& findings) {
    if (!initialized_) {
        return {"Agent not initialized"};
    }
    
    std::ostringstream oss;
    oss << "Extract the key insights from the following findings. "
        << "List each insight as a concise bullet point:\n\n";
    oss << findings << "\n";
    
    std::string response = llm_->generate(oss.str(), 256, 0.7f, 40, 0.9f);
    
    // Parse insights
    std::vector<std::string> insights;
    std::regex insight_regex(R"((?:^|\n)(?:[-*•]|\d+\.)\s*(.+?)(?=\n(?:[-*•]|\d+\.)|$))", 
                            std::regex::multiline);
    std::sregex_iterator iter(response.begin(), response.end(), insight_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string insight = iter->str(1);
        insight.erase(0, insight.find_first_not_of(" \t\n\r"));
        insight.erase(insight.find_last_not_of(" \t\n\r") + 1);
        if (!insight.empty()) {
            insights.push_back(insight);
        }
    }
    
    // If no insights found, use the whole response
    if (insights.empty() && !response.empty()) {
        insights.push_back(response.substr(0, 200));
    }
    
    return insights;
}

std::string Agent::generateSummary(const memory::TraceEntry& entry) {
    std::ostringstream oss;
    oss << "Task: " << entry.task_keyword << "\n";
    oss << "Reasoning steps: " << entry.reasoning_steps.size() << "\n";
    oss << "Key findings: " << entry.findings.substr(0, 150);
    if (entry.findings.length() > 150) {
        oss << "...";
    }
    return oss.str();
}

memory::TraceEntry Agent::ruminate(const std::string& task_keyword) {
    memory::TraceEntry entry;
    entry.task_keyword = task_keyword;
    
    // Step 1: Generate initial reasoning steps
    entry.reasoning_steps = generateReasoningSteps(task_keyword);
    
    // Step 2: Reflect on each step
    std::vector<std::string> reflections;
    for (const auto& step : entry.reasoning_steps) {
        std::string reflection = reflectOnStep(step);
        reflections.push_back(reflection);
        // Update reasoning step with reflection
        entry.reasoning_steps.push_back("Reflection: " + reflection);
    }
    
    // Step 3: Synthesize findings
    entry.findings = synthesizeFindings(entry.reasoning_steps);
    
    // Step 4: Extract key insights
    entry.key_insights = extractKeyInsights(entry.findings);
    
    // Step 5: Generate summary
    entry.summary = generateSummary(entry);
    
    // Step 6: Update world model
    updateWorldModel(entry.findings);
    
    // Add to trace
    trace_manager_->addTrace(entry);
    
    return entry;
}

std::string Agent::processTask(const std::string& task_keyword) {
    memory::TraceEntry entry = ruminate(task_keyword);
    
    // Generate report
    std::ostringstream report;
    report << "=== Agent " << agent_id_ << " Report ===\n\n";
    report << "Task: " << task_keyword << "\n\n";
    report << "Reasoning Steps:\n";
    for (size_t i = 0; i < entry.reasoning_steps.size(); ++i) {
        report << (i + 1) << ". " << entry.reasoning_steps[i] << "\n";
    }
    report << "\nFindings:\n" << entry.findings << "\n\n";
    report << "Key Insights:\n";
    for (const auto& insight : entry.key_insights) {
        report << "- " << insight << "\n";
    }
    report << "\n";
    
    return report.str();
}

void Agent::updateWorldModel(const std::string& information) {
    if (world_model_.empty() || world_model_ == "Initial world model state") {
        world_model_ = information;
    } else {
        world_model_ += "\n\n" + information;
    }
    
    // Limit world model size
    const size_t MAX_WORLD_MODEL_SIZE = 8192;
    if (world_model_.length() > MAX_WORLD_MODEL_SIZE) {
        world_model_ = world_model_.substr(world_model_.length() - MAX_WORLD_MODEL_SIZE);
    }
}

void Agent::handleMessage(const communication::Message& message) {
    // Process incoming message
    // Could trigger new tasks, update world model, etc.
    switch (message.type) {
        case communication::MessageType::TASK_DELEGATION:
            // Process delegated task
            processTask(message.payload);
            break;
        case communication::MessageType::FINDINGS_SHARING:
            // Incorporate findings into world model
            updateWorldModel("From " + message.from_agent_id + ": " + message.payload);
            break;
        case communication::MessageType::QUERY:
            // Respond to query
            // Could send response back
            break;
    }
}

void Agent::sendMessage(const std::string& to_agent_id,
                       communication::MessageType message_type,
                       const std::string& payload) {
    if (agent_manager_) {
        communication::Message message;
        message.from_agent_id = agent_id_;
        message.to_agent_id = to_agent_id;
        message.type = message_type;
        message.payload = payload;
        message.timestamp = std::chrono::system_clock::now();
        
        agent_manager_->routeMessage(message);
    }
}

std::vector<memory::TraceEntry> Agent::getTraceHistory() const {
    return trace_manager_->getTraceHistory();
}

std::string Agent::getNormalizedContext() const {
    return trace_manager_->getNormalizedContext();
}

} // namespace agent

