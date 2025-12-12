#include "memory.hpp"
#include <algorithm>
#include <sstream>
#include <regex>
#include <cmath>
#include <cstring>

namespace memory {

// MDLEncoder Implementation

MDLEncoder::MDLEncoder() {
    // Initialize with common English words for better compression
    token_frequencies_["the"] = 1000;
    token_frequencies_["be"] = 800;
    token_frequencies_["to"] = 700;
    token_frequencies_["of"] = 600;
    token_frequencies_["and"] = 500;
}

std::vector<std::string> MDLEncoder::tokenize(const std::string& context) {
    std::vector<std::string> tokens;
    std::regex word_regex(R"(\b\w+\b)");
    std::sregex_iterator iter(context.begin(), context.end(), word_regex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string token = iter->str();
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        tokens.push_back(token);
    }
    
    return tokens;
}

void MDLEncoder::updateTokenFrequencies(const std::string& context) {
    auto tokens = tokenize(context);
    for (const auto& token : tokens) {
        token_frequencies_[token]++;
    }
}

std::vector<std::pair<std::string, size_t>> MDLEncoder::extractPatterns(const std::string& context) {
    std::vector<std::pair<std::string, size_t>> patterns;
    
    // Extract n-grams (2-grams to 5-grams)
    for (size_t n = 2; n <= 5; ++n) {
        std::map<std::string, size_t> ngram_counts;
        
        auto tokens = tokenize(context);
        for (size_t i = 0; i + n <= tokens.size(); ++i) {
            std::ostringstream oss;
            for (size_t j = 0; j < n; ++j) {
                if (j > 0) oss << " ";
                oss << tokens[i + j];
            }
            std::string ngram = oss.str();
            ngram_counts[ngram]++;
        }
        
        // Keep patterns that appear multiple times
        for (const auto& pair : ngram_counts) {
            if (pair.second >= 2 && pair.first.length() >= MIN_PATTERN_LENGTH) {
                patterns.push_back({pair.first, pair.second});
            }
        }
    }
    
    // Sort by frequency (descending)
    std::sort(patterns.begin(), patterns.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    // Limit to top patterns
    if (patterns.size() > 50) {
        patterns.resize(50);
    }
    
    return patterns;
}

std::string MDLEncoder::replacePatterns(const std::string& context,
                                       const std::vector<std::pair<std::string, size_t>>& patterns) {
    std::string result = context;
    
    // Create pattern codes (short identifiers)
    std::map<std::string, std::string> pattern_codes;
    char code_char = 'A';
    
    for (const auto& pattern : patterns) {
        if (code_char > 'Z') break; // Limit codes
        
        std::string code = std::string(1, code_char) + std::string(1, code_char);
        pattern_codes[pattern.first] = code;
        code_char++;
    }
    
    // Replace patterns with codes
    for (const auto& pair : pattern_codes) {
        size_t pos = 0;
        while ((pos = result.find(pair.first, pos)) != std::string::npos) {
            // Only replace if it saves space
            if (pair.second.length() < pair.first.length()) {
                result.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length();
            } else {
                pos += pair.first.length();
            }
        }
    }
    
    return result;
}

std::string MDLEncoder::compress(const std::string& context) {
    if (context.empty()) return context;
    
    updateTokenFrequencies(context);
    auto patterns = extractPatterns(context);
    return replacePatterns(context, patterns);
}

std::string MDLEncoder::encode(const std::string& context) {
    // MDL encoding: compress and normalize
    std::string compressed = compress(context);
    
    // Additional normalization: remove redundant whitespace
    std::regex whitespace_regex(R"(\s+)");
    compressed = std::regex_replace(compressed, whitespace_regex, " ");
    
    // Trim
    compressed.erase(0, compressed.find_first_not_of(" \t\n\r"));
    compressed.erase(compressed.find_last_not_of(" \t\n\r") + 1);
    
    return compressed;
}

std::string MDLEncoder::decode(const std::string& encoded) {
    // For now, decoding is mostly reversing whitespace normalization
    // Pattern replacement would need a reverse mapping
    return encoded;
}

size_t MDLEncoder::calculateDescriptionLength(const std::string& context) {
    // Approximate description length using entropy
    if (context.empty()) return 0;
    
    std::map<char, size_t> char_counts;
    for (char c : context) {
        char_counts[c]++;
    }
    
    // Calculate entropy
    double entropy = 0.0;
    size_t total_chars = context.length();
    
    for (const auto& pair : char_counts) {
        double prob = static_cast<double>(pair.second) / total_chars;
        if (prob > 0.0) {
            entropy -= prob * std::log2(prob);
        }
    }
    
    // Description length ≈ entropy * length (in bits)
    return static_cast<size_t>(entropy * total_chars);
}

// TraceManager Implementation

TraceManager::TraceManager(size_t trace_limit, size_t memory_size)
    : trace_limit_(trace_limit)
    , memory_size_(memory_size)
{
}

void TraceManager::addTrace(const TraceEntry& entry) {
    traces_.push_back(entry);
    
    // Check if we need to compress old traces
    if (isTraceLimitReached() || calculateMemoryUsage() > memory_size_) {
        compressOldTraces();
    }
}

std::vector<TraceEntry> TraceManager::getTraceHistory() const {
    std::vector<TraceEntry> result;
    result.reserve(traces_.size());
    
    for (const auto& trace : traces_) {
        result.push_back(trace);
    }
    
    return result;
}

std::vector<std::string> TraceManager::getKeyInsights() const {
    std::vector<std::string> insights;
    
    for (const auto& trace : traces_) {
        insights.insert(insights.end(),
                       trace.key_insights.begin(),
                       trace.key_insights.end());
    }
    
    return insights;
}

std::vector<std::string> TraceManager::getSummaries() const {
    std::vector<std::string> summaries = compressed_summaries_;
    
    // Also include summaries from current traces if they exist
    for (const auto& trace : traces_) {
        if (!trace.summary.empty()) {
            summaries.push_back(trace.summary);
        }
    }
    
    return summaries;
}

std::string TraceManager::getNormalizedContext() const {
    std::ostringstream oss;
    
    // Add compressed summaries first
    for (const auto& summary : compressed_summaries_) {
        oss << "[Summary] " << encoder_.encode(summary) << "\n";
    }
    
    // Add recent traces (full details)
    for (const auto& trace : traces_) {
        oss << "[Trace: " << trace.task_keyword << "]\n";
        
        if (!trace.summary.empty()) {
            oss << "Summary: " << encoder_.encode(trace.summary) << "\n";
        }
        
        if (!trace.key_insights.empty()) {
            oss << "Insights: ";
            for (const auto& insight : trace.key_insights) {
                oss << encoder_.encode(insight) << "; ";
            }
            oss << "\n";
        }
        
        if (!trace.findings.empty()) {
            oss << "Findings: " << encoder_.encode(trace.findings) << "\n";
        }
    }
    
    return encoder_.encode(oss.str());
}

bool TraceManager::isTraceLimitReached() const {
    return traces_.size() >= trace_limit_;
}

void TraceManager::clear() {
    traces_.clear();
    compressed_summaries_.clear();
}

std::string TraceManager::createSummary(const TraceEntry& entry) const {
    std::ostringstream oss;
    oss << "Task: " << entry.task_keyword << ". ";
    
    if (!entry.findings.empty()) {
        oss << "Findings: " << entry.findings.substr(0, 200);
        if (entry.findings.length() > 200) {
            oss << "...";
        }
    }
    
    if (!entry.key_insights.empty()) {
        oss << " Key insights: " << entry.key_insights.size() << " items.";
    }
    
    return oss.str();
}

size_t TraceManager::calculateMemoryUsage() const {
    size_t usage = 0;
    
    for (const auto& trace : traces_) {
        usage += trace.task_keyword.size();
        usage += trace.findings.size();
        usage += trace.summary.size();
        
        for (const auto& step : trace.reasoning_steps) {
            usage += step.size();
        }
        
        for (const auto& insight : trace.key_insights) {
            usage += insight.size();
        }
    }
    
    for (const auto& summary : compressed_summaries_) {
        usage += summary.size();
    }
    
    return usage;
}

void TraceManager::compressOldTraces() {
    // Compress oldest traces until we're under the limit
    while (traces_.size() > trace_limit_ / 2) {
        TraceEntry oldest = traces_.front();
        traces_.pop_front();
        
        std::string summary = createSummary(oldest);
        compressed_summaries_.push_back(summary);
        
        // Limit compressed summaries too
        if (compressed_summaries_.size() > trace_limit_) {
            compressed_summaries_.pop_front();
        }
    }
    
    // Also check memory size
    while (calculateMemoryUsage() > memory_size_ && !traces_.empty()) {
        TraceEntry oldest = traces_.front();
        traces_.pop_front();
        
        std::string summary = createSummary(oldest);
        compressed_summaries_.push_back(summary);
    }
}

} // namespace memory

