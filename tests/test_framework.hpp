/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * No MIT License
 * 
 * Comprehensive test framework: Unit, Integration, Regression, Blackbox, A-B, UX tests
 */

#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <chrono>
#include <memory>
#include <iostream>
#include <sstream>
#include <cassert>

namespace testing {

/**
 * Test result
 */
enum class TestResult {
    PASSED,
    FAILED,
    SKIPPED,
    ERROR
};

/**
 * Test case
 */
class TestCase {
public:
    TestCase(const std::string& name, std::function<void()> test_func);
    
    /**
     * Run test
     */
    TestResult run();
    
    /**
     * Get test name
     */
    const std::string& getName() const { return name_; }
    
    /**
     * Get execution time
     */
    std::chrono::milliseconds getExecutionTime() const { return execution_time_; }
    
    /**
     * Get error message
     */
    const std::string& getErrorMessage() const { return error_message_; }

private:
    std::string name_;
    std::function<void()> test_func_;
    TestResult result_;
    std::chrono::milliseconds execution_time_;
    std::string error_message_;
};

/**
 * Test suite
 */
class TestSuite {
public:
    TestSuite(const std::string& name);
    
    /**
     * Add test case
     */
    void addTest(const std::string& name, std::function<void()> test_func);
    
    /**
     * Run all tests
     */
    void run();
    
    /**
     * Get test results
     */
    std::vector<TestResult> getResults() const;
    
    /**
     * Get statistics
     */
    void printStatistics() const;

private:
    std::string name_;
    std::vector<std::unique_ptr<TestCase>> tests_;
    std::vector<TestResult> results_;
};

/**
 * Test runner
 */
class TestRunner {
public:
    TestRunner();
    
    /**
     * Register test suite
     */
    void registerSuite(const std::string& name, std::shared_ptr<TestSuite> suite);
    
    /**
     * Run all tests
     */
    void runAll();
    
    /**
     * Run specific suite
     */
    void runSuite(const std::string& suite_name);
    
    /**
     * Generate report
     */
    void generateReport() const;

private:
    std::map<std::string, std::shared_ptr<TestSuite>> suites_;
    size_t total_tests_;
    size_t passed_tests_;
    size_t failed_tests_;
};

/**
 * Assertion macros
 */
#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: " #condition); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))
#define ASSERT_EQ(expected, actual) ASSERT_TRUE((expected) == (actual))
#define ASSERT_NE(expected, actual) ASSERT_TRUE((expected) != (actual))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))

/**
 * Test registration macro
 */
#define REGISTER_TEST(suite, name) \
    suite->addTest(name, [&]() {

#define END_TEST \
    });

} // namespace testing

#endif // TEST_FRAMEWORK_HPP

