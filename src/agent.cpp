#include "agent.hpp"
#include "agent_manager.hpp"
#include "verbose.hpp"
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
    verbose::logPhase("AGENT INITIALIZATION: " + agent_id_);
    
    if (initialized_) {
        verbose::log("Agent already initialized", "Agent Init");
        return true;
    }
    
    verbose::logStep("Agent Init", 1, "Loading model: " + model_path_);
    // Load model - this may take time for Ollama models (HTTP request)
    // or file-based models (file I/O)
    if (!llm_->loadModel(model_path_)) {
        verbose::log("Model loading failed", "Agent Init");
        return false;
    }
    verbose::logStep("Agent Init", 2, "Model loaded successfully");
    
    verbose::logStep("Agent Init", 3, "Marking agent as initialized");
    initialized_ = true;
    verbose::log("Agent initialization completed", "Agent Init");
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
    verbose::logStep("Reasoning", 1, "Building prompt for reasoning steps");
    if (!initialized_) {
        verbose::log("Agent not initialized", "Reasoning");
        return {"Agent not initialized"};
    }
    
    std::string prompt = buildPrompt(task_keyword);
    prompt += "Break down the task into reasoning steps. List each step clearly.\n";
    verbose::log("Prompt built (" + std::to_string(prompt.length()) + " chars)", "Reasoning");
    
    verbose::logStep("Reasoning", 2, "Calling LLM to generate reasoning steps");
    std::cout << "\n[THINKING] " << agent_id_ << " is generating reasoning steps...\n" << std::flush;
    std::string response = llm_->generate(prompt, 512, 0.7f, 40, 0.9f);
    verbose::log("LLM response received (" + std::to_string(response.length()) + " chars)", "Reasoning");
    
    // Show the raw LLM response for thinking process
    if (verbose::enabled) {
        std::cout << "\n[THINKING OUTPUT - Reasoning Steps]\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << response << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        std::cout.flush();
    }
    
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
        verbose::log("No steps found in response, creating default step", "Reasoning");
        steps.push_back("Initial analysis of task: " + task_keyword);
    }
    
    verbose::log("Parsed " + std::to_string(steps.size()) + " reasoning steps", "Reasoning");
    return steps;
}

std::string Agent::reflectOnStep(const std::string& step) {
    verbose::log("Reflecting on step: " + step.substr(0, 50) + "...", "Reflection");
    if (!initialized_) {
        verbose::log("Agent not initialized", "Reflection");
        return "Agent not initialized";
    }
    
    std::ostringstream oss;
    oss << "Reflect on this reasoning step: " << step << "\n";
    oss << "Consider: Is this step valid? What are the implications? "
        << "What questions does this raise?\n";
    
    verbose::log("Calling LLM for reflection", "Reflection");
    std::cout << "[THINKING] " << agent_id_ << " is reflecting on: " 
              << step.substr(0, 60) << (step.length() > 60 ? "..." : "") << "\n" << std::flush;
    std::string reflection = llm_->generate(oss.str(), 256, 0.7f, 40, 0.9f);
    verbose::log("Reflection received (" + std::to_string(reflection.length()) + " chars)", "Reflection");
    
    // Show reflection output
    if (verbose::enabled) {
        std::cout << "[REFLECTION OUTPUT]\n";
        std::cout << "  " << reflection << "\n\n";
        std::cout.flush();
    }
    return reflection;
}

std::string Agent::synthesizeFindings(const std::vector<std::string>& reasoning_steps) {
    verbose::logStep("Synthesis", 1, "Building synthesis prompt from " + std::to_string(reasoning_steps.size()) + " steps");
    if (!initialized_) {
        verbose::log("Agent not initialized", "Synthesis");
        return "Agent not initialized";
    }
    
    std::ostringstream oss;
    oss << "Based on the following reasoning steps, synthesize the key findings:\n\n";
    
    for (size_t i = 0; i < reasoning_steps.size(); ++i) {
        oss << "Step " << (i + 1) << ": " << reasoning_steps[i] << "\n";
    }
    
    oss << "\nProvide a comprehensive summary of findings.\n";
    
    verbose::logStep("Synthesis", 2, "Calling LLM to synthesize findings");
    std::cout << "[THINKING] " << agent_id_ << " is synthesizing findings from " 
              << reasoning_steps.size() << " reasoning steps...\n" << std::flush;
    std::string findings = llm_->generate(oss.str(), 512, 0.7f, 40, 0.9f);
    verbose::log("Findings synthesized (" + std::to_string(findings.length()) + " chars)", "Synthesis");
    
    // Show synthesized findings
    if (verbose::enabled) {
        std::cout << "\n[SYNTHESIS OUTPUT - Findings]\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << findings << "\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        std::cout.flush();
    }
    return findings;
}

std::vector<std::string> Agent::extractKeyInsights(const std::string& findings) {
    verbose::logStep("Insight Extraction", 1, "Building insight extraction prompt");
    if (!initialized_) {
        verbose::log("Agent not initialized", "Insight Extraction");
        return {"Agent not initialized"};
    }
    
    std::ostringstream oss;
    oss << "Extract the key insights from the following findings. "
        << "List each insight as a concise bullet point:\n\n";
    oss << findings << "\n";
    
    verbose::logStep("Insight Extraction", 2, "Calling LLM to extract insights");
    std::cout << "[THINKING] " << agent_id_ << " is extracting key insights...\n" << std::flush;
    std::string response = llm_->generate(oss.str(), 256, 0.7f, 40, 0.9f);
    verbose::log("LLM response received for insights", "Insight Extraction");
    
    // Show insights extraction output
    if (verbose::enabled) {
        std::cout << "[INSIGHTS OUTPUT]\n";
        std::cout << "  " << response << "\n\n";
        std::cout.flush();
    }
    
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
    verbose::logPhase("RUMINATION: " + agent_id_ + " - Task: " + task_keyword);
    
    memory::TraceEntry entry;
    entry.task_keyword = task_keyword;
    
    verbose::logStep("Rumination", 1, "Generating initial reasoning steps");
    // Step 1: Generate initial reasoning steps
    entry.reasoning_steps = generateReasoningSteps(task_keyword);
    verbose::log("Generated " + std::to_string(entry.reasoning_steps.size()) + " reasoning steps", "Rumination");
    
    verbose::logStep("Rumination", 2, "Reflecting on reasoning steps");
    // Step 2: Reflect on each step
    std::vector<std::string> reflections;
    for (size_t i = 0; i < entry.reasoning_steps.size(); ++i) {
        verbose::log("Reflecting on step " + std::to_string(i+1) + "/" + std::to_string(entry.reasoning_steps.size()), "Rumination");
        std::string reflection = reflectOnStep(entry.reasoning_steps[i]);
        reflections.push_back(reflection);
        // Update reasoning step with reflection
        entry.reasoning_steps.push_back("Reflection: " + reflection);
    }
    verbose::log("Completed reflections on all steps", "Rumination");
    
    verbose::logStep("Rumination", 3, "Synthesizing findings");
    // Step 3: Synthesize findings
    entry.findings = synthesizeFindings(entry.reasoning_steps);
    verbose::log("Findings synthesized (" + std::to_string(entry.findings.length()) + " chars)", "Rumination");
    
    verbose::logStep("Rumination", 4, "Extracting key insights");
    // Step 4: Extract key insights
    entry.key_insights = extractKeyInsights(entry.findings);
    verbose::log("Extracted " + std::to_string(entry.key_insights.size()) + " key insights", "Rumination");
    
    verbose::logStep("Rumination", 5, "Generating summary");
    // Step 5: Generate summary
    entry.summary = generateSummary(entry);
    verbose::log("Summary generated", "Rumination");
    
    verbose::logStep("Rumination", 6, "Updating world model");
    // Step 6: Update world model
    updateWorldModel(entry.findings);
    verbose::log("World model updated", "Rumination");
    
    verbose::logStep("Rumination", 7, "Adding trace to memory");
    // Add to trace
    trace_manager_->addTrace(entry);
    verbose::log("Trace added to memory", "Rumination");
    
    verbose::log("Rumination completed successfully", "Rumination");
    return entry;
}

std::string Agent::processTask(const std::string& task_keyword) {
    verbose::logPhase("TASK PROCESSING: " + agent_id_ + " - " + task_keyword);
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "🤖 AGENT: " << agent_id_ << " | TASK: " << task_keyword << "\n";
    std::cout << std::string(70, '=') << "\n\n";
    std::cout.flush();
    
    verbose::logStep("Task Processing", 1, "Starting rumination");
    memory::TraceEntry entry = ruminate(task_keyword);
    verbose::logStep("Task Processing", 2, "Rumination completed, generating report");
    
    // Generate report
    std::ostringstream report;
    report << "\n" << std::string(70, '=') << "\n";
    report << "📊 FINAL REPORT - Agent: " << agent_id_ << "\n";
    report << std::string(70, '=') << "\n\n";
    report << "Task: " << task_keyword << "\n\n";
    report << "Reasoning Steps (" << entry.reasoning_steps.size() << " total):\n";
    report << std::string(70, '-') << "\n";
    for (size_t i = 0; i < entry.reasoning_steps.size(); ++i) {
        report << (i + 1) << ". " << entry.reasoning_steps[i] << "\n";
    }
    report << "\n" << std::string(70, '-') << "\n";
    report << "\nFindings:\n";
    report << std::string(70, '-') << "\n";
    report << entry.findings << "\n";
    report << std::string(70, '-') << "\n";
    report << "\nKey Insights (" << entry.key_insights.size() << " total):\n";
    report << std::string(70, '-') << "\n";
    for (const auto& insight : entry.key_insights) {
        report << "• " << insight << "\n";
    }
    report << std::string(70, '-') << "\n";
    report << "\n" << std::string(70, '=') << "\n\n";
    
    verbose::log("Report generated (" + std::to_string(report.str().length()) + " chars)", "Task Processing");
    verbose::log("Task processing completed", "Task Processing");
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
        case communication::MessageType::RESPONSE:
            // Handle response to a query
            updateWorldModel("Response from " + message.from_agent_id + ": " + message.payload);
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

