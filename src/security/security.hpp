/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * 
 * Security layer for input validation, sanitization, and encryption
 */

#ifndef SECURITY_HPP
#define SECURITY_HPP

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <map>

namespace security {

/**
 * Input validator with recursive retry mechanism
 */
class InputValidator {
public:
    InputValidator(size_t max_retries = 3);
    
    /**
     * Validate and sanitize input with retry mechanism
     * @param input Raw input string
     * @param validator Validation function
     * @param sanitizer Sanitization function
     * @return Validated and sanitized input, or empty if validation fails
     */
    std::string validateWithRetry(const std::string& input,
                                  std::function<bool(const std::string&)> validator,
                                  std::function<std::string(const std::string&)> sanitizer);
    
    /**
     * Validate task keyword
     */
    bool validateTaskKeyword(const std::string& keyword);
    
    /**
     * Validate agent ID
     */
    bool validateAgentId(const std::string& agent_id);
    
    /**
     * Sanitize input string
     */
    std::string sanitize(const std::string& input);
    
    /**
     * Check for SQL injection patterns
     */
    bool checkSQLInjection(const std::string& input);
    
    /**
     * Check for XSS patterns
     */
    bool checkXSS(const std::string& input);
    
    /**
     * Check for command injection patterns
     */
    bool checkCommandInjection(const std::string& input);
    
    /**
     * Validate file path
     */
    bool validateFilePath(const std::string& path);

private:
    size_t max_retries_;
    std::unordered_set<std::string> allowed_chars_;
    std::mutex mutex_;
    
    /**
     * Recursive validation with retry
     */
    std::string validateRecursive(const std::string& input,
                                  std::function<bool(const std::string&)> validator,
                                  std::function<std::string(const std::string&)> sanitizer,
                                  size_t attempt);
};

/**
 * Encryption service for data at rest and in transit
 */
class EncryptionService {
public:
    EncryptionService();
    ~EncryptionService();
    
    /**
     * Encrypt data
     * @param plaintext Data to encrypt
     * @param key Encryption key (optional, uses default if empty)
     * @return Encrypted data (base64 encoded)
     */
    std::string encrypt(const std::string& plaintext, const std::string& key = "");
    
    /**
     * Decrypt data
     * @param ciphertext Encrypted data (base64 encoded)
     * @param key Decryption key (optional, uses default if empty)
     * @return Decrypted data
     */
    std::string decrypt(const std::string& ciphertext, const std::string& key = "");
    
    /**
     * Generate encryption key
     */
    std::string generateKey(size_t length = 32);
    
    /**
     * Hash data (SHA-256)
     */
    std::string hash(const std::string& data);
    
    /**
     * Verify hash
     */
    bool verifyHash(const std::string& data, const std::string& hash);

private:
    std::string default_key_;
    std::mutex mutex_;
    
    /**
     * Simple XOR encryption (replace with proper encryption in production)
     */
    std::string xorEncrypt(const std::string& data, const std::string& key);
    std::string xorDecrypt(const std::string& data, const std::string& key);
    
    /**
     * Base64 encode/decode
     */
    std::string base64Encode(const std::string& data);
    std::string base64Decode(const std::string& data);
};

/**
 * Secure communication wrapper
 */
class SecureChannel {
public:
    SecureChannel(std::shared_ptr<EncryptionService> encryption);
    
    /**
     * Send encrypted message
     */
    std::string sendSecure(const std::string& message);
    
    /**
     * Receive and decrypt message
     */
    std::string receiveSecure(const std::string& encrypted_message);
    
    /**
     * Establish secure session
     */
    bool establishSession(const std::string& peer_id);

private:
    std::shared_ptr<EncryptionService> encryption_;
    std::map<std::string, std::string> session_keys_;
    std::mutex mutex_;
};

} // namespace security

#endif // SECURITY_HPP

