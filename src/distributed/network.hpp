/*
 * Copyright (C) 2025, Shyamal Chandra
 * No MIT License
 * 
 * Distributed system support: Network communication, remote agents
 */

#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace distributed {

/**
 * Network endpoint
 */
struct Endpoint {
    std::string host;
    uint16_t port;
    
    Endpoint() : port(0) {}
    Endpoint(const std::string& h, uint16_t p) : host(h), port(p) {}
    
    std::string toString() const {
        return host + ":" + std::to_string(port);
    }
};

/**
 * Network message
 */
struct NetworkMessage {
    std::string from_agent_id;
    std::string to_agent_id;
    std::string message_type;
    std::string payload;
    std::string message_id;
    uint64_t timestamp;
    
    NetworkMessage() : timestamp(0) {}
    
    std::string serialize() const;
    static NetworkMessage deserialize(const std::string& data);
};

/**
 * TCP client for network communication
 */
class TCPClient {
public:
    TCPClient();
    ~TCPClient();
    
    /**
     * Connect to server
     */
    bool connect(const Endpoint& endpoint);
    
    /**
     * Disconnect
     */
    void disconnect();
    
    /**
     * Send message
     */
    bool send(const std::string& data);
    
    /**
     * Receive message
     */
    std::string receive(size_t max_size = 4096);
    
    /**
     * Check if connected
     */
    bool isConnected() const { return connected_; }

private:
    int socket_fd_;
    bool connected_;
    Endpoint endpoint_;
    std::mutex mutex_;
};

/**
 * TCP server for receiving connections
 */
class TCPServer {
public:
    TCPServer(uint16_t port);
    ~TCPServer();
    
    /**
     * Start listening
     */
    bool start();
    
    /**
     * Stop listening
     */
    void stop();
    
    /**
     * Accept connection
     */
    std::unique_ptr<TCPClient> accept();
    
    /**
     * Set message handler
     */
    void setMessageHandler(std::function<void(const NetworkMessage&)> handler);

private:
    uint16_t port_;
    int server_fd_;
    std::atomic<bool> running_;
    std::function<void(const NetworkMessage&)> message_handler_;
    std::thread accept_thread_;
    std::mutex mutex_;
    
    void acceptLoop();
};

/**
 * Distributed agent registry
 */
class AgentRegistry {
public:
    AgentRegistry();
    
    /**
     * Register remote agent
     */
    bool registerAgent(const std::string& agent_id, const Endpoint& endpoint);
    
    /**
     * Unregister agent
     */
    void unregisterAgent(const std::string& agent_id);
    
    /**
     * Get agent endpoint
     */
    Endpoint getEndpoint(const std::string& agent_id) const;
    
    /**
     * Get all registered agents
     */
    std::vector<std::string> getRegisteredAgents() const;

private:
    std::map<std::string, Endpoint> agents_;
    std::mutex mutex_;
};

/**
 * Message router for distributed system
 */
class DistributedRouter {
public:
    DistributedRouter(std::shared_ptr<AgentRegistry> registry);
    
    /**
     * Route message to remote agent
     */
    bool routeMessage(const NetworkMessage& message);
    
    /**
     * Broadcast message to all agents
     */
    void broadcast(const NetworkMessage& message);

private:
    std::shared_ptr<AgentRegistry> registry_;
    std::map<std::string, std::unique_ptr<TCPClient>> connections_;
    std::mutex mutex_;
    
    TCPClient* getConnection(const std::string& agent_id);
};

} // namespace distributed

#endif // NETWORK_HPP

