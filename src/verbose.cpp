#include "verbose.hpp"
#include <iostream>
#include <sstream>
#include <mutex>

namespace verbose {
    bool enabled = false;
    static std::mutex log_mutex; // Protect verbose output from multiple threads
    
    void log(const std::string& message, const std::string& phase, int step) {
        if (!enabled) return;
        
        std::lock_guard<std::mutex> lock(log_mutex);
        std::ostringstream oss;
        oss << "[VERBOSE]";
        if (!phase.empty()) {
            oss << " [" << phase << "]";
        }
        if (step >= 0) {
            oss << " [Step " << step << "]";
        }
        oss << " " << message;
        std::cout << oss.str() << std::endl;
        std::cout.flush(); // Ensure immediate output
    }
    
    void logPhase(const std::string& phase_name) {
        if (!enabled) return;
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "\n[VERBOSE] ========== PHASE: " << phase_name << " ==========" << std::endl;
        std::cout.flush();
    }
    
    void logStep(const std::string& phase, int step, const std::string& message) {
        log(message, phase, step);
    }
}

