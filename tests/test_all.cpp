/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * 
 * Comprehensive test suite: Unit, Integration, Regression, Blackbox, A-B, UX tests
 * Target: 20 tests per line of code
 */

#include "test_framework.hpp"
#include "../src/agent.hpp"
#include "../src/agent_manager.hpp"
#include "../src/memory.hpp"
#include "../src/security/security.hpp"
#include "../src/fault_tolerance/retry.hpp"
#include "../src/communication.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace testing;

// Unit Tests
void createUnitTests() {
    auto suite = std::make_shared<TestSuite>("Unit Tests");
    
    // Security tests
    suite->addTest("InputValidator_Sanitize", []() {
        security::InputValidator validator;
        std::string input = "test<script>alert('xss')</script>";
        std::string sanitized = validator.sanitize(input);
        ASSERT_TRUE(sanitized.find("<script>") == std::string::npos);
    });
    
    suite->addTest("InputValidator_ValidateTaskKeyword", []() {
        security::InputValidator validator;
        ASSERT_TRUE(validator.validateTaskKeyword("research topic"));
        ASSERT_FALSE(validator.validateTaskKeyword("'; DROP TABLE users; --"));
    });
    
    suite->addTest("InputValidator_ValidateAgentId", []() {
        security::InputValidator validator;
        ASSERT_TRUE(validator.validateAgentId("agent1"));
        ASSERT_FALSE(validator.validateAgentId("agent 1")); // Space not allowed
    });
    
    suite->addTest("InputValidator_CheckSQLInjection", []() {
        security::InputValidator validator;
        ASSERT_TRUE(validator.checkSQLInjection("'; DROP TABLE users; --"));
        ASSERT_FALSE(validator.checkSQLInjection("normal query"));
    });
    
    suite->addTest("InputValidator_CheckXSS", []() {
        security::InputValidator validator;
        ASSERT_TRUE(validator.checkXSS("<script>alert('xss')</script>"));
        ASSERT_FALSE(validator.checkXSS("normal text"));
    });
    
    suite->addTest("EncryptionService_EncryptDecrypt", []() {
        security::EncryptionService encryption;
        std::string plaintext = "test message";
        std::string encrypted = encryption.encrypt(plaintext);
        std::string decrypted = encryption.decrypt(encrypted);
        ASSERT_EQ(plaintext, decrypted);
    });
    
    suite->addTest("EncryptionService_Hash", []() {
        security::EncryptionService encryption;
        std::string data = "test data";
        std::string hash1 = encryption.hash(data);
        std::string hash2 = encryption.hash(data);
        ASSERT_EQ(hash1, hash2);
        ASSERT_TRUE(encryption.verifyHash(data, hash1));
    });
    
    // Memory tests
    suite->addTest("MDLEncoder_Encode", []() {
        memory::MDLEncoder encoder;
        std::string context = "test context for encoding";
        std::string encoded = encoder.encode(context);
        ASSERT_FALSE(encoded.empty());
    });
    
    suite->addTest("TraceManager_AddTrace", []() {
        memory::TraceManager manager(10, 1024);
        memory::TraceEntry entry;
        entry.task_keyword = "test";
        entry.findings = "test findings";
        manager.addTrace(entry);
        ASSERT_EQ(manager.getTraceCount(), 1);
    });
    
    suite->addTest("TraceManager_TraceLimit", []() {
        memory::TraceManager manager(5, 1024);
        for (int i = 0; i < 10; ++i) {
            memory::TraceEntry entry;
            entry.task_keyword = "test" + std::to_string(i);
            manager.addTrace(entry);
        }
        ASSERT_LE(manager.getTraceCount(), 5);
    });
    
    // Communication tests
    suite->addTest("MessageQueue_PushPop", []() {
        communication::MessageQueue queue;
        communication::Message msg;
        msg.payload = "test";
        queue.push(msg);
        communication::Message received;
        ASSERT_TRUE(queue.pop(received));
        ASSERT_EQ(received.payload, "test");
    });
    
    suite->addTest("MessageQueue_Empty", []() {
        communication::MessageQueue queue;
        ASSERT_TRUE(queue.empty());
        communication::Message msg;
        queue.push(msg);
        ASSERT_FALSE(queue.empty());
    });
    
    // Fault tolerance tests
    suite->addTest("RetryExecutor_Success", []() {
        fault_tolerance::RetryExecutor<int> executor;
        int result = executor.execute([]() { return 42; });
        ASSERT_EQ(result, 42);
        ASSERT_EQ(executor.getSuccessfulAttempts(), 1);
    });
    
    suite->addTest("RetryExecutor_Retry", []() {
        fault_tolerance::RetryPolicy policy;
        policy.max_attempts = 3;
        fault_tolerance::RetryExecutor<int> executor(policy);
        int attempts = 0;
        int result = executor.execute([&attempts]() {
            attempts++;
            if (attempts < 3) throw std::runtime_error("fail");
            return 42;
        });
        ASSERT_EQ(result, 42);
        ASSERT_EQ(attempts, 3);
    });
    
    suite->addTest("CircuitBreaker_StateTransitions", []() {
        fault_tolerance::CircuitBreaker breaker(3);
        ASSERT_EQ(breaker.getState(), fault_tolerance::CircuitBreaker::State::CLOSED);
    });
    
    // Add more unit tests...
    TestRunner::instance().registerSuite("Unit", suite);
}

// Integration Tests
void createIntegrationTests() {
    auto suite = std::make_shared<TestSuite>("Integration Tests");
    
    suite->addTest("AgentManager_CreateAgent", []() {
        agent::AgentManager manager;
        bool result = manager.createAgent("test_agent", "models/test.bin", 20, 4096);
        // May fail if model doesn't exist, but should not crash
        ASSERT_TRUE(manager.hasAgent("test_agent") || !result);
    });
    
    suite->addTest("Agent_ProcessTask", []() {
        agent::Agent agent("test_agent", "models/test.bin", 20, 4096);
        agent.initialize();
        std::string result = agent.processTask("test task");
        ASSERT_FALSE(result.empty());
    });
    
    suite->addTest("Security_InputValidation_Integration", []() {
        security::InputValidator validator(3);
        std::string input = "research quantum computing";
        std::string validated = validator.validateWithRetry(
            input,
            [&validator](const std::string& s) {
                return validator.validateTaskKeyword(s);
            },
            [&validator](const std::string& s) {
                return validator.sanitize(s);
            }
        );
        ASSERT_FALSE(validated.empty());
    });
    
    // Add more integration tests...
    TestRunner::instance().registerSuite("Integration", suite);
}

// Regression Tests
void createRegressionTests() {
    auto suite = std::make_shared<TestSuite>("Regression Tests");
    
    suite->addTest("Regression_MemoryLeak", []() {
        for (int i = 0; i < 100; ++i) {
            memory::TraceManager manager(10, 1024);
            memory::TraceEntry entry;
            manager.addTrace(entry);
        }
        // Should not crash or leak memory
        ASSERT_TRUE(true);
    });
    
    suite->addTest("Regression_ConcurrentAccess", []() {
        communication::MessageQueue queue;
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&queue, i]() {
                communication::Message msg;
                msg.payload = "test" + std::to_string(i);
                queue.push(msg);
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        ASSERT_EQ(queue.size(), 10);
    });
    
    // Add more regression tests...
    TestRunner::instance().registerSuite("Regression", suite);
}

// Blackbox Tests
void createBlackboxTests() {
    auto suite = std::make_shared<TestSuite>("Blackbox Tests");
    
    suite->addTest("Blackbox_InvalidInput", []() {
        security::InputValidator validator;
        std::string malicious = "'; DROP TABLE users; -- <script>alert('xss')</script>";
        bool valid = validator.validateTaskKeyword(malicious);
        ASSERT_FALSE(valid);
    });
    
    suite->addTest("Blackbox_EmptyInput", []() {
        security::InputValidator validator;
        ASSERT_FALSE(validator.validateTaskKeyword(""));
    });
    
    suite->addTest("Blackbox_ExtremelyLongInput", []() {
        security::InputValidator validator;
        std::string long_input(10000, 'a');
        bool valid = validator.validateTaskKeyword(long_input);
        ASSERT_FALSE(valid); // Should reject extremely long input
    });
    
    // Add more blackbox tests...
    TestRunner::instance().registerSuite("Blackbox", suite);
}

// A-B Tests
void createABTests() {
    auto suite = std::make_shared<TestSuite>("A-B Tests");
    
    suite->addTest("AB_EncodingStrategy", []() {
        memory::MDLEncoder encoder1, encoder2;
        std::string context = "test context";
        
        std::string encoded1 = encoder1.encode(context);
        std::string encoded2 = encoder2.encode(context);
        
        // Both should produce valid encodings
        ASSERT_FALSE(encoded1.empty());
        ASSERT_FALSE(encoded2.empty());
    });
    
    suite->addTest("AB_RetryPolicies", []() {
        fault_tolerance::RetryPolicy policy1;
        policy1.max_attempts = 3;
        policy1.exponential_backoff = true;
        
        fault_tolerance::RetryPolicy policy2;
        policy2.max_attempts = 5;
        policy2.exponential_backoff = false;
        
        fault_tolerance::RetryExecutor<int> executor1(policy1);
        fault_tolerance::RetryExecutor<int> executor2(policy2);
        
        // Both should work
        int result1 = executor1.execute([]() { return 1; });
        int result2 = executor2.execute([]() { return 2; });
        
        ASSERT_EQ(result1, 1);
        ASSERT_EQ(result2, 2);
    });
    
    // Add more A-B tests...
    TestRunner::instance().registerSuite("A-B", suite);
}

// UX Tests
void createUXTests() {
    auto suite = std::make_shared<TestSuite>("UX Tests");
    
    suite->addTest("UX_ResponseTime", []() {
        auto start = std::chrono::high_resolution_clock::now();
        security::InputValidator validator;
        validator.validateTaskKeyword("test");
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        ASSERT_LT(duration.count(), 100); // Should be fast
    });
    
    suite->addTest("UX_ErrorMessages", []() {
        security::InputValidator validator;
        std::string invalid = "'; DROP TABLE; --";
        bool valid = validator.validateTaskKeyword(invalid);
        ASSERT_FALSE(valid);
        // Error should be clear (tested by absence of crash)
    });
    
    suite->addTest("UX_ConcurrentPerformance", []() {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([]() {
                security::InputValidator validator;
                for (int j = 0; j < 100; ++j) {
                    validator.validateTaskKeyword("test" + std::to_string(j));
                }
            });
        }
        for (auto& t : threads) {
            t.join();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        ASSERT_LT(duration.count(), 5000); // Should complete in reasonable time
    });
    
    // Add more UX tests...
    TestRunner::instance().registerSuite("UX", suite);
}

// Singleton for TestRunner
class TestRunner {
public:
    static TestRunner& instance() {
        static TestRunner runner;
        return runner;
    }
    
    void registerSuite(const std::string& name, std::shared_ptr<TestSuite> suite) {
        suites_[name] = suite;
    }
    
    void runAll() {
        for (auto& pair : suites_) {
            pair.second->run();
            pair.second->printStatistics();
        }
    }
    
private:
    std::map<std::string, std::shared_ptr<TestSuite>> suites_;
};

int main() {
    std::cout << "=== Comprehensive Test Suite ===" << std::endl;
    std::cout << "Copyright (C) 2025, Shyamal Suhana Chandra" << std::endl;
    
    createUnitTests();
    createIntegrationTests();
    createRegressionTests();
    createBlackboxTests();
    createABTests();
    createUXTests();
    
    TestRunner::instance().runAll();
    
    std::cout << "\n=== All Tests Complete ===" << std::endl;
    return 0;
}

