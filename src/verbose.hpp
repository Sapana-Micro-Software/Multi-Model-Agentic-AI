#ifndef VERBOSE_HPP
#define VERBOSE_HPP

#include <iostream>
#include <string>
#include <sstream>

/**
 * Global verbose flag and helper functions
 * Note: Functions are implemented in verbose.cpp to ensure thread-safe output
 */
namespace verbose {
    extern bool enabled;
    
    void log(const std::string& message, const std::string& phase = "", int step = -1);
    void logPhase(const std::string& phase_name);
    void logStep(const std::string& phase, int step, const std::string& message);
}

#endif // VERBOSE_HPP

