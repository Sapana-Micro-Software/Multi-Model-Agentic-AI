/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * No MIT License
 */

#include "test_framework.hpp"
#include <iomanip>

namespace testing {

TestCase::TestCase(const std::string& name, std::function<void()> test_func)
    : name_(name)
    , test_func_(test_func)
    , result_(TestResult::SKIPPED)
    , execution_time_(0)
{
}

TestResult TestCase::run() {
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        test_func_();
        result_ = TestResult::PASSED;
    } catch (const std::exception& e) {
        result_ = TestResult::FAILED;
        error_message_ = e.what();
    } catch (...) {
        result_ = TestResult::ERROR;
        error_message_ = "Unknown error";
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    execution_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    return result_;
}

TestSuite::TestSuite(const std::string& name)
    : name_(name)
{
}

void TestSuite::addTest(const std::string& name, std::function<void()> test_func) {
    tests_.push_back(std::make_unique<TestCase>(name, test_func));
}

void TestSuite::run() {
    results_.clear();
    std::cout << "Running test suite: " << name_ << std::endl;
    
    for (auto& test : tests_) {
        TestResult result = test->run();
        results_.push_back(result);
        
        std::string status;
        switch (result) {
            case TestResult::PASSED:
                status = "PASSED";
                break;
            case TestResult::FAILED:
                status = "FAILED";
                break;
            case TestResult::ERROR:
                status = "ERROR";
                break;
            default:
                status = "SKIPPED";
        }
        
        std::cout << "  [" << status << "] " << test->getName();
        std::cout << " (" << test->getExecutionTime().count() << "ms)";
        if (result == TestResult::FAILED || result == TestResult::ERROR) {
            std::cout << " - " << test->getErrorMessage();
        }
        std::cout << std::endl;
    }
}

std::vector<TestResult> TestSuite::getResults() const {
    return results_;
}

void TestSuite::printStatistics() const {
    size_t passed = 0, failed = 0, error = 0, skipped = 0;
    for (const auto& result : results_) {
        switch (result) {
            case TestResult::PASSED: passed++; break;
            case TestResult::FAILED: failed++; break;
            case TestResult::ERROR: error++; break;
            default: skipped++;
        }
    }
    
    std::cout << "\nTest Suite Statistics: " << name_ << std::endl;
    std::cout << "  Total: " << results_.size() << std::endl;
    std::cout << "  Passed: " << passed << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    std::cout << "  Errors: " << error << std::endl;
    std::cout << "  Skipped: " << skipped << std::endl;
}

TestRunner::TestRunner()
    : total_tests_(0)
    , passed_tests_(0)
    , failed_tests_(0)
{
}

void TestRunner::registerSuite(const std::string& name, std::shared_ptr<TestSuite> suite) {
    suites_[name] = suite;
}

void TestRunner::runAll() {
    std::cout << "=== Running All Test Suites ===" << std::endl;
    
    for (auto& pair : suites_) {
        pair.second->run();
        pair.second->printStatistics();
        
        auto results = pair.second->getResults();
        for (const auto& result : results) {
            total_tests_++;
            if (result == TestResult::PASSED) {
                passed_tests_++;
            } else if (result == TestResult::FAILED || result == TestResult::ERROR) {
                failed_tests_++;
            }
        }
    }
    
    generateReport();
}

void TestRunner::runSuite(const std::string& suite_name) {
    auto it = suites_.find(suite_name);
    if (it != suites_.end()) {
        it->second->run();
        it->second->printStatistics();
    }
}

void TestRunner::generateReport() const {
    std::cout << "\n=== Test Report ===" << std::endl;
    std::cout << "Total Tests: " << total_tests_ << std::endl;
    std::cout << "Passed: " << passed_tests_ << std::endl;
    std::cout << "Failed: " << failed_tests_ << std::endl;
    std::cout << "Success Rate: " << std::fixed << std::setprecision(2)
              << (total_tests_ > 0 ? (100.0 * passed_tests_ / total_tests_) : 0.0)
              << "%" << std::endl;
}

} // namespace testing

