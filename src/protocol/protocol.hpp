/*
 * Copyright (C) 2025, Shyamal Chandra
 * No MIT License
 * 
 * Protocol-driven communication with formal message protocols
 */

#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cstdint>

namespace protocol {

/**
 * Protocol version
 */
struct ProtocolVersion {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    
    ProtocolVersion() : major(1), minor(0), patch(0) {}
    std::string toString() const {
        return std::to_string(major) + "." + 
               std::to_string(minor) + "." + 
               std::to_string(patch);
    }
};

/**
 * Message header
 */
struct MessageHeader {
    uint32_t magic; // Protocol magic number
    ProtocolVersion version;
    uint32_t message_type;
    uint32_t payload_length;
    uint64_t message_id;
    uint64_t timestamp;
    std::string from_agent_id;
    std::string to_agent_id;
    
    MessageHeader() 
        : magic(0x4D414C4C) // "MALL" in hex
        , message_type(0)
        , payload_length(0)
        , message_id(0)
        , timestamp(0)
    {}
    
    std::vector<uint8_t> serialize() const;
    static MessageHeader deserialize(const std::vector<uint8_t>& data);
    bool validate() const;
};

/**
 * Protocol message
 */
class ProtocolMessage {
public:
    ProtocolMessage();
    ProtocolMessage(uint32_t type, const std::string& payload);
    
    /**
     * Serialize message
     */
    std::vector<uint8_t> serialize() const;
    
    /**
     * Deserialize message
     */
    static ProtocolMessage deserialize(const std::vector<uint8_t>& data);
    
    /**
     * Validate message
     */
    bool validate() const;
    
    MessageHeader header;
    std::string payload;
    
    // Message types
    static constexpr uint32_t MSG_TASK = 0x0001;
    static constexpr uint32_t MSG_RESPONSE = 0x0002;
    static constexpr uint32_t MSG_FINDINGS = 0x0003;
    static constexpr uint32_t MSG_QUERY = 0x0004;
    static constexpr uint32_t MSG_CACHE_INVALIDATE = 0x0005;
    static constexpr uint32_t MSG_CACHE_REQUEST = 0x0006;
    static constexpr uint32_t MSG_HEARTBEAT = 0x0007;
    static constexpr uint32_t MSG_ERROR = 0xFFFF;
};

/**
 * Protocol handler
 */
class ProtocolHandler {
public:
    ProtocolHandler();
    
    /**
     * Register message handler
     */
    void registerHandler(uint32_t message_type,
                        std::function<void(const ProtocolMessage&)> handler);
    
    /**
     * Handle incoming message
     */
    bool handleMessage(const ProtocolMessage& message);
    
    /**
     * Create message
     */
    ProtocolMessage createMessage(uint32_t type,
                                 const std::string& from,
                                 const std::string& to,
                                 const std::string& payload);

private:
    std::map<uint32_t, std::function<void(const ProtocolMessage&)>> handlers_;
    std::atomic<uint64_t> message_id_counter_;
    std::mutex mutex_;
};

/**
 * Protocol validator
 */
class ProtocolValidator {
public:
    /**
     * Validate message format
     */
    static bool validateFormat(const ProtocolMessage& message);
    
    /**
     * Validate message content
     */
    static bool validateContent(const ProtocolMessage& message);
    
    /**
     * Check protocol version compatibility
     */
    static bool checkVersionCompatibility(const ProtocolVersion& version);
};

} // namespace protocol

#endif // PROTOCOL_HPP

