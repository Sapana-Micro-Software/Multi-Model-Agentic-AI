#include "reporting.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace reporting {

// ConsoleReporter Implementation

ConsoleReporter::ConsoleReporter(bool use_stderr)
    : use_stderr_(use_stderr)
{
}

std::string ConsoleReporter::formatReport(const Report& report) const {
    std::ostringstream oss;
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(report.timestamp);
    oss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] ";
    
    oss << "=== Agent " << report.agent_id << " Report ===\n\n";
    oss << "Task: " << report.task_keyword << "\n\n";
    
    if (!report.reasoning_steps.empty()) {
        oss << "Reasoning Steps:\n";
        for (size_t i = 0; i < report.reasoning_steps.size(); ++i) {
            oss << "  " << (i + 1) << ". " << report.reasoning_steps[i] << "\n";
        }
        oss << "\n";
    }
    
    if (!report.findings.empty()) {
        oss << "Findings:\n" << report.findings << "\n\n";
    }
    
    if (!report.key_insights.empty()) {
        oss << "Key Insights:\n";
        for (const auto& insight : report.key_insights) {
            oss << "  - " << insight << "\n";
        }
        oss << "\n";
    }
    
    oss << "---\n\n";
    
    return oss.str();
}

void ConsoleReporter::report(const Report& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string formatted = formatReport(report);
    
    if (use_stderr_) {
        std::cerr << formatted;
    } else {
        std::cout << formatted;
    }
}

void ConsoleReporter::report(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (use_stderr_) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

void ConsoleReporter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (use_stderr_) {
        std::cerr.flush();
    } else {
        std::cout.flush();
    }
}

// FileReporter Implementation

FileReporter::FileReporter(const std::string& file_path, bool per_agent)
    : base_file_path_(file_path)
    , per_agent_(per_agent)
{
    if (!per_agent_ && !file_path.empty()) {
        shared_file_ = std::make_unique<std::ofstream>(file_path, std::ios::app);
        if (!shared_file_->is_open()) {
            shared_file_.reset();
        }
    }
}

FileReporter::~FileReporter() {
    flush();
    agent_files_.clear();
    shared_file_.reset();
}

std::ofstream& FileReporter::getFileForAgent(const std::string& agent_id) {
    if (!per_agent_) {
        if (shared_file_ && shared_file_->is_open()) {
            return *shared_file_;
        }
        // Fallback: create default file
        if (!shared_file_) {
            shared_file_ = std::make_unique<std::ofstream>("agent_reports.log", std::ios::app);
        }
        return *shared_file_;
    }
    
    // Per-agent files
    auto it = agent_files_.find(agent_id);
    if (it == agent_files_.end()) {
        std::string file_path = base_file_path_.empty() 
            ? "agent_" + agent_id + ".log"
            : base_file_path_ + "_" + agent_id + ".log";
        
        auto file = std::make_unique<std::ofstream>(file_path, std::ios::app);
        if (file->is_open()) {
            agent_files_[agent_id] = std::move(file);
            it = agent_files_.find(agent_id);
        } else {
            // Fallback to shared file
            if (!shared_file_) {
                shared_file_ = std::make_unique<std::ofstream>("agent_reports.log", std::ios::app);
            }
            return *shared_file_;
        }
    }
    
    return *it->second;
}

std::string FileReporter::formatReport(const Report& report) const {
    std::ostringstream oss;
    
    // Format timestamp
    auto time_t = std::chrono::system_clock::to_time_t(report.timestamp);
    oss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] ";
    
    oss << "Agent: " << report.agent_id << " | Task: " << report.task_keyword << "\n";
    
    if (!report.reasoning_steps.empty()) {
        oss << "Reasoning Steps:\n";
        for (size_t i = 0; i < report.reasoning_steps.size(); ++i) {
            oss << "  " << (i + 1) << ". " << report.reasoning_steps[i] << "\n";
        }
    }
    
    if (!report.findings.empty()) {
        oss << "Findings: " << report.findings << "\n";
    }
    
    if (!report.key_insights.empty()) {
        oss << "Key Insights: ";
        for (size_t i = 0; i < report.key_insights.size(); ++i) {
            oss << report.key_insights[i];
            if (i < report.key_insights.size() - 1) {
                oss << "; ";
            }
        }
        oss << "\n";
    }
    
    oss << "---\n";
    
    return oss.str();
}

void FileReporter::report(const Report& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream& file = getFileForAgent(report.agent_id);
    if (file.is_open()) {
        file << formatReport(report) << std::endl;
    }
}

void FileReporter::report(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (shared_file_ && shared_file_->is_open()) {
        *shared_file_ << message << std::endl;
    } else if (!agent_files_.empty()) {
        // Write to first available file
        *agent_files_.begin()->second << message << std::endl;
    }
}

void FileReporter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : agent_files_) {
        if (pair.second && pair.second->is_open()) {
            pair.second->flush();
        }
    }
    
    if (shared_file_ && shared_file_->is_open()) {
        shared_file_->flush();
    }
}

// CallbackReporter Implementation

CallbackReporter::CallbackReporter(CallbackFunc callback)
    : report_callback_(std::move(callback))
{
}

CallbackReporter::CallbackReporter(MessageCallbackFunc callback)
    : message_callback_(std::move(callback))
{
}

CallbackReporter::CallbackReporter(CallbackFunc report_callback, MessageCallbackFunc message_callback)
    : report_callback_(std::move(report_callback))
    , message_callback_(std::move(message_callback))
{
}

void CallbackReporter::report(const Report& report) {
    if (report_callback_) {
        report_callback_(report);
    } else if (message_callback_) {
        // Convert report to string
        std::ostringstream oss;
        oss << "Agent " << report.agent_id << ": " << report.task_keyword << " - " << report.findings;
        message_callback_(oss.str());
    }
}

void CallbackReporter::report(const std::string& message) {
    if (message_callback_) {
        message_callback_(message);
    }
}

// CompositeReporter Implementation

void CompositeReporter::addReporter(std::unique_ptr<Reporter> reporter) {
    std::lock_guard<std::mutex> lock(mutex_);
    reporters_.push_back(std::move(reporter));
}

void CompositeReporter::report(const Report& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& reporter : reporters_) {
        reporter->report(report);
    }
}

void CompositeReporter::report(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& reporter : reporters_) {
        reporter->report(message);
    }
}

void CompositeReporter::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& reporter : reporters_) {
        reporter->flush();
    }
}

} // namespace reporting

