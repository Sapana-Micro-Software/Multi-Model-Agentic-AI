#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <chrono>
#include <memory>
#include <cstdint>

namespace memory {

/**
 * Represents a single trace entry in agent's working memory
 */
struct TraceEntry {
    std::chrono::system_clock::time_point timestamp;
    std::string task_keyword;
    std::vector<std::string> reasoning_steps;
    std::string findings;
    std::string summary;
    std::vector<std::string> key_insights;
    
    TraceEntry() : timestamp(std::chrono::system_clock::now()) {}
};

/**
 * MDL Encoder for context normalization
 * Implements custom encoding scheme to minimize description length
 */
class MDLEncoder {
public:
    MDLEncoder();
    
    /**
     * Encode context using MDL principles
     * @param context Input context string
     * @return Encoded context with minimized description length
     */
    std::string encode(const std::string& context);
    
    /**
     * Decode MDL-encoded context
     * @param encoded Encoded context
     * @return Decoded context
     */
    std::string decode(const std::string& encoded);
    
    /**
     * Compress context by identifying patterns
     * @param context Input context
     * @return Compressed context
     */
    std::string compress(const std::string& context);
    
    /**
     * Calculate description length of context
     * @param context Input context
     * @return Description length in bits (approximate)
     */
    size_t calculateDescriptionLength(const std::string& context);
    
    /**
     * Update token frequencies for better encoding
     * @param context Context to analyze
     */
    void updateTokenFrequencies(const std::string& context);

private:
    // Token frequency map for pattern recognition
    std::map<std::string, size_t> token_frequencies_;
    
    // Common patterns cache
    std::map<std::string, std::string> pattern_cache_;
    
    // Minimum pattern length to consider
    static constexpr size_t MIN_PATTERN_LENGTH = 3;
    static constexpr size_t MAX_PATTERN_LENGTH = 50;
    
    /**
     * Extract common patterns from context
     */
    std::vector<std::pair<std::string, size_t>> extractPatterns(const std::string& context);
    
    /**
     * Tokenize context into words/tokens
     */
    std::vector<std::string> tokenize(const std::string& context);
    
    /**
     * Replace patterns with shorter codes
     */
    std::string replacePatterns(const std::string& context, 
                               const std::vector<std::pair<std::string, size_t>>& patterns);
};

/**
 * Trace Manager for working memory with recursion limits
 */
class TraceManager {
public:
    explicit TraceManager(size_t trace_limit = 20, size_t memory_size = 4096);
    
    /**
     * Add a new trace entry
     * @param entry Trace entry to add
     */
    void addTrace(const TraceEntry& entry);
    
    /**
     * Get trace history (respecting recursion limit)
     * @return Vector of trace entries
     */
    std::vector<TraceEntry> getTraceHistory() const;
    
    /**
     * Get key insights from trace history
     * @return Vector of key insights
     */
    std::vector<std::string> getKeyInsights() const;
    
    /**
     * Get summaries of old traces (compressed)
     * @return Vector of summaries
     */
    std::vector<std::string> getSummaries() const;
    
    /**
     * Get full context for LLM (MDL-normalized)
     * @return Normalized context string
     */
    std::string getNormalizedContext() const;
    
    /**
     * Check if trace limit is reached
     */
    bool isTraceLimitReached() const;
    
    /**
     * Get current trace count
     */
    size_t getTraceCount() const { return traces_.size(); }
    
    /**
     * Set trace limit
     */
    void setTraceLimit(size_t limit) { trace_limit_ = limit; }
    
    /**
     * Set memory size limit
     */
    void setMemorySize(size_t size) { memory_size_ = size; }
    
    /**
     * Clear all traces
     */
    void clear();
    
    /**
     * Compress oldest traces into summaries when limit reached
     */
    void compressOldTraces();

private:
    std::deque<TraceEntry> traces_;
    std::vector<std::string> compressed_summaries_;
    size_t trace_limit_;
    size_t memory_size_;
    MDLEncoder encoder_;
    
    /**
     * Calculate current memory usage
     */
    size_t calculateMemoryUsage() const;
    
    /**
     * Create summary from trace entry
     */
    std::string createSummary(const TraceEntry& entry) const;
};

} // namespace memory

#endif // MEMORY_HPP

