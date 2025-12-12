#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <string>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <map>

namespace communication {

/**
 * Message types for inter-agent communication
 */
enum class MessageType {
    TASK_DELEGATION,    // Delegate a task to another agent
    FINDINGS_SHARING,   // Share findings with another agent
    QUERY,              // Query another agent
    RESPONSE            // Response to a query
};

/**
 * Message structure for agent communication
 */
struct Message {
    std::string from_agent_id;
    std::string to_agent_id;
    MessageType type;
    std::string payload;
    std::chrono::system_clock::time_point timestamp;
    
    Message()
        : type(MessageType::QUERY)
        , timestamp(std::chrono::system_clock::now())
    {}
    
    Message(const std::string& from,
            const std::string& to,
            MessageType msg_type,
            const std::string& msg_payload)
        : from_agent_id(from)
        , to_agent_id(to)
        , type(msg_type)
        , payload(msg_payload)
        , timestamp(std::chrono::system_clock::now())
    {}
};

/**
 * Thread-safe message queue for agent communication
 */
class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;
    
    // Disable copy
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    
    /**
     * Push a message to the queue
     * @param message Message to add
     */
    void push(const Message& message);
    
    /**
     * Pop a message from the queue (blocking)
     * @param message Output message
     * @return true if message was retrieved, false if queue was closed
     */
    bool pop(Message& message);
    
    /**
     * Try to pop a message without blocking
     * @param message Output message
     * @return true if message was retrieved, false if queue is empty
     */
    bool tryPop(Message& message);
    
    /**
     * Check if queue is empty
     */
    bool empty() const;
    
    /**
     * Get queue size
     */
    size_t size() const;
    
    /**
     * Close the queue (stops blocking on pop)
     */
    void close();
    
    /**
     * Check if queue is closed
     */
    bool isClosed() const;

private:
    std::queue<Message> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool closed_ = false;
};

/**
 * Message router for directing messages to agents
 */
class MessageRouter {
public:
    MessageRouter() = default;
    ~MessageRouter() = default;
    
    /**
     * Register an agent's message queue
     * @param agent_id Agent identifier
     * @param queue Pointer to agent's message queue
     */
    void registerAgent(const std::string& agent_id, MessageQueue* queue);
    
    /**
     * Unregister an agent
     * @param agent_id Agent identifier
     */
    void unregisterAgent(const std::string& agent_id);
    
    /**
     * Route a message to the target agent
     * @param message Message to route
     * @return true if message was routed successfully, false if agent not found
     */
    bool routeMessage(const Message& message);
    
    /**
     * Broadcast a message to all registered agents
     * @param message Message to broadcast
     * @param exclude_agent_id Agent ID to exclude from broadcast
     */
    void broadcast(const Message& message, const std::string& exclude_agent_id = "");

private:
    std::map<std::string, MessageQueue*> agent_queues_;
    std::mutex mutex_;
};

} // namespace communication

#endif // COMMUNICATION_HPP

