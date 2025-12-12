#ifndef REPORTING_HPP
#define REPORTING_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <fstream>
#include <mutex>
#include <chrono>
#include <map>

namespace reporting {

/**
 * Report data structure
 */
struct Report {
    std::string agent_id;
    std::string task_keyword;
    std::string findings;
    std::vector<std::string> reasoning_steps;
    std::vector<std::string> key_insights;
    std::chrono::system_clock::time_point timestamp;
    
    Report()
        : timestamp(std::chrono::system_clock::now())
    {}
};

/**
 * Abstract base class for reporters
 */
class Reporter {
public:
    virtual ~Reporter() = default;
    
    /**
     * Report findings
     * @param report Report data
     */
    virtual void report(const Report& report) = 0;
    
    /**
     * Report a simple message
     * @param message Message to report
     */
    virtual void report(const std::string& message) = 0;
    
    /**
     * Flush any buffered output
     */
    virtual void flush() {}
};

/**
 * Console reporter - outputs to stdout/stderr
 */
class ConsoleReporter : public Reporter {
public:
    ConsoleReporter(bool use_stderr = false);
    
    void report(const Report& report) override;
    void report(const std::string& message) override;
    void flush() override;

private:
    bool use_stderr_;
    std::mutex mutex_;
    
    /**
     * Format report as string
     */
    std::string formatReport(const Report& report) const;
};

/**
 * File reporter - outputs to file(s)
 */
class FileReporter : public Reporter {
public:
    /**
     * Constructor
     * @param file_path File path (if empty, uses per-agent files)
     * @param per_agent If true, creates separate file per agent
     */
    FileReporter(const std::string& file_path = "", bool per_agent = false);
    
    ~FileReporter();
    
    void report(const Report& report) override;
    void report(const std::string& message) override;
    void flush() override;

private:
    std::string base_file_path_;
    bool per_agent_;
    std::map<std::string, std::unique_ptr<std::ofstream>> agent_files_;
    std::unique_ptr<std::ofstream> shared_file_;
    std::mutex mutex_;
    
    /**
     * Get file stream for agent
     */
    std::ofstream& getFileForAgent(const std::string& agent_id);
    
    /**
     * Format report as string
     */
    std::string formatReport(const Report& report) const;
};

/**
 * Callback reporter - uses user-defined callback function
 */
class CallbackReporter : public Reporter {
public:
    /**
     * Callback function type
     */
    using CallbackFunc = std::function<void(const Report&)>;
    using MessageCallbackFunc = std::function<void(const std::string&)>;
    
    /**
     * Constructor with report callback
     */
    explicit CallbackReporter(CallbackFunc callback);
    
    /**
     * Constructor with message callback
     */
    explicit CallbackReporter(MessageCallbackFunc callback);
    
    /**
     * Constructor with both callbacks
     */
    CallbackReporter(CallbackFunc report_callback, MessageCallbackFunc message_callback);
    
    void report(const Report& report) override;
    void report(const std::string& message) override;

private:
    CallbackFunc report_callback_;
    MessageCallbackFunc message_callback_;
};

/**
 * Composite reporter - combines multiple reporters
 */
class CompositeReporter : public Reporter {
public:
    CompositeReporter() = default;
    
    /**
     * Add a reporter
     */
    void addReporter(std::unique_ptr<Reporter> reporter);
    
    void report(const Report& report) override;
    void report(const std::string& message) override;
    void flush() override;

private:
    std::vector<std::unique_ptr<Reporter>> reporters_;
    std::mutex mutex_;
};

} // namespace reporting

#endif // REPORTING_HPP

