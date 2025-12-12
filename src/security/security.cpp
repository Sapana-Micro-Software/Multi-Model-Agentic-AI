/*
 * Copyright (C) 2025, Shyamal Chandra
 * No MIT License
 */

#include "security.hpp"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <cstring>

namespace security {

// InputValidator Implementation

InputValidator::InputValidator(size_t max_retries)
    : max_retries_(max_retries)
{
    // Initialize allowed characters
    for (char c = 'a'; c <= 'z'; ++c) allowed_chars_.insert(std::string(1, c));
    for (char c = 'A'; c <= 'Z'; ++c) allowed_chars_.insert(std::string(1, c));
    for (char c = '0'; c <= '9'; ++c) allowed_chars_.insert(std::string(1, c));
    allowed_chars_.insert("_");
    allowed_chars_.insert("-");
    allowed_chars_.insert(" ");
    allowed_chars_.insert(".");
    allowed_chars_.insert("/");
}

std::string InputValidator::validateWithRetry(const std::string& input,
                                              std::function<bool(const std::string&)> validator,
                                              std::function<std::string(const std::string&)> sanitizer) {
    return validateRecursive(input, validator, sanitizer, 0);
}

std::string InputValidator::validateRecursive(const std::string& input,
                                             std::function<bool(const std::string&)> validator,
                                             std::function<std::string(const std::string&)> sanitizer,
                                             size_t attempt) {
    if (attempt >= max_retries_) {
        return ""; // Max retries exceeded
    }
    
    // Sanitize input
    std::string sanitized = sanitizer ? sanitizer(input) : sanitize(input);
    
    // Validate
    if (validator && validator(sanitized)) {
        return sanitized;
    }
    
    // Retry with additional sanitization
    if (attempt < max_retries_ - 1) {
        std::string more_sanitized = sanitize(sanitized);
        return validateRecursive(more_sanitized, validator, sanitizer, attempt + 1);
    }
    
    return ""; // Validation failed
}

bool InputValidator::validateTaskKeyword(const std::string& keyword) {
    if (keyword.empty() || keyword.length() > 1000) {
        return false;
    }
    
    if (checkSQLInjection(keyword) || checkXSS(keyword) || checkCommandInjection(keyword)) {
        return false;
    }
    
    return true;
}

bool InputValidator::validateAgentId(const std::string& agent_id) {
    if (agent_id.empty() || agent_id.length() > 100) {
        return false;
    }
    
    std::regex valid_id(R"([a-zA-Z0-9_-]+)");
    return std::regex_match(agent_id, valid_id);
}

std::string InputValidator::sanitize(const std::string& input) {
    std::string result = input;
    
    // Remove null bytes
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());
    
    // Remove control characters except newline and tab
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](char c) {
                                    return (c < 32 && c != '\n' && c != '\t');
                                }),
                 result.end());
    
    // Trim whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

bool InputValidator::checkSQLInjection(const std::string& input) {
    std::vector<std::string> patterns = {
        "';", "--", "/*", "*/", "xp_", "sp_", "exec", "union", "select"
    };
    
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    for (const auto& pattern : patterns) {
        if (lower.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool InputValidator::checkXSS(const std::string& input) {
    std::vector<std::string> patterns = {
        "<script", "</script>", "javascript:", "onerror=", "onclick="
    };
    
    std::string lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    
    for (const auto& pattern : patterns) {
        if (lower.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool InputValidator::checkCommandInjection(const std::string& input) {
    std::vector<std::string> patterns = {
        ";", "|", "&", "`", "$(", "${{", ">", "<", "\n"
    };
    
    for (const auto& pattern : patterns) {
        if (input.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

bool InputValidator::validateFilePath(const std::string& path) {
    if (path.empty() || path.length() > 512) {
        return false;
    }
    
    // Check for path traversal
    if (path.find("..") != std::string::npos) {
        return false;
    }
    
    // Check for absolute paths (optional restriction)
    if (path[0] == '/') {
        // Allow absolute paths but validate
    }
    
    return true;
}

// EncryptionService Implementation

EncryptionService::EncryptionService() {
    default_key_ = generateKey();
}

EncryptionService::~EncryptionService() = default;

std::string EncryptionService::encrypt(const std::string& plaintext, const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string use_key = key.empty() ? default_key_ : key;
    std::string encrypted = xorEncrypt(plaintext, use_key);
    return base64Encode(encrypted);
}

std::string EncryptionService::decrypt(const std::string& ciphertext, const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string use_key = key.empty() ? default_key_ : key;
    std::string decoded = base64Decode(ciphertext);
    return xorDecrypt(decoded, use_key);
}

std::string EncryptionService::generateKey(size_t length) {
    std::string key;
    key.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        key += static_cast<char>('A' + (rand() % 26));
    }
    
    return key;
}

std::string EncryptionService::hash(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.length());
    SHA256_Final(hash, &sha256);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    
    return oss.str();
}

bool EncryptionService::verifyHash(const std::string& data, const std::string& hash) {
    std::string computed_hash = this->hash(data);
    return computed_hash == hash;
}

std::string EncryptionService::xorEncrypt(const std::string& data, const std::string& key) {
    std::string result = data;
    size_t key_len = key.length();
    
    for (size_t i = 0; i < result.length(); ++i) {
        result[i] ^= key[i % key_len];
    }
    
    return result;
}

std::string EncryptionService::xorDecrypt(const std::string& data, const std::string& key) {
    return xorEncrypt(data, key); // XOR is symmetric
}

std::string EncryptionService::base64Encode(const std::string& data) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (result.length() % 4) {
        result.push_back('=');
    }
    
    return result;
}

std::string EncryptionService::base64Decode(const std::string& data) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -8;
    
    for (char c : data) {
        if (c == '=') break;
        
        const char* pos = std::strchr(base64_chars, c);
        if (pos == nullptr) continue;
        
        val = (val << 6) + (pos - base64_chars);
        valb += 6;
        
        if (valb >= 0) {
            result.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return result;
}

// SecureChannel Implementation

SecureChannel::SecureChannel(std::shared_ptr<EncryptionService> encryption)
    : encryption_(encryption)
{
}

std::string SecureChannel::sendSecure(const std::string& message) {
    if (!encryption_) {
        return message;
    }
    return encryption_->encrypt(message);
}

std::string SecureChannel::receiveSecure(const std::string& encrypted_message) {
    if (!encryption_) {
        return encrypted_message;
    }
    return encryption_->decrypt(encrypted_message);
}

bool SecureChannel::establishSession(const std::string& peer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!encryption_) {
        return false;
    }
    session_keys_[peer_id] = encryption_->generateKey();
    return true;
}

} // namespace security

