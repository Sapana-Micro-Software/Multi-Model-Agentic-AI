/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * 
 * Cache coherence system with distributed caching
 */

#ifndef CACHE_COHERENT_HPP
#define CACHE_COHERENT_HPP

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <vector>
#include <functional>
#include <atomic>

namespace cache {

/**
 * Cache entry with metadata
 */
template<typename T>
struct CacheEntry {
    T data;
    std::chrono::system_clock::time_point timestamp;
    std::chrono::milliseconds ttl;
    std::string version;
    bool dirty;
    
    CacheEntry() : dirty(false), ttl(std::chrono::milliseconds(0)) {}
    
    bool isExpired() const {
        if (ttl.count() == 0) return false;
        auto now = std::chrono::system_clock::now();
        return (now - timestamp) > ttl;
    }
};

/**
 * Cache coherence protocol states
 */
enum class CoherenceState {
    INVALID,
    SHARED,
    EXCLUSIVE,
    MODIFIED,
    OWNED
};

/**
 * Cache line with coherence state
 */
template<typename T>
struct CacheLine {
    std::string key;
    CacheEntry<T> entry;
    CoherenceState state;
    std::vector<std::string> sharers; // Agent IDs sharing this cache line
    std::string owner; // Agent ID owning this cache line
    
    CacheLine() : state(CoherenceState::INVALID) {}
};

/**
 * MESI-like cache coherence protocol
 */
template<typename T>
class CoherentCache {
public:
    CoherentCache(const std::string& agent_id);
    
    /**
     * Get value from cache
     */
    bool get(const std::string& key, T& value);
    
    /**
     * Put value into cache
     */
    void put(const std::string& key, const T& value,
            std::chrono::milliseconds ttl = std::chrono::milliseconds(0));
    
    /**
     * Invalidate cache entry
     */
    void invalidate(const std::string& key);
    
    /**
     * Handle coherence message
     */
    void handleCoherenceMessage(const std::string& message_type,
                               const std::string& key,
                               const std::string& from_agent);
    
    /**
     * Set coherence callback for remote operations
     */
    void setCoherenceCallback(std::function<void(const std::string&,
                                                 const std::string&,
                                                 const std::string&)> callback);

private:
    std::string agent_id_;
    std::map<std::string, CacheLine<T>> cache_;
    std::mutex mutex_;
    std::function<void(const std::string&, const std::string&, const std::string&)> coherence_callback_;
    
    /**
     * Request shared access
     */
    void requestShared(const std::string& key);
    
    /**
     * Request exclusive access
     */
    void requestExclusive(const std::string& key);
    
    /**
     * Upgrade to modified
     */
    void upgradeToModified(const std::string& key);
};

/**
 * Distributed cache manager
 */
class DistributedCache {
public:
    DistributedCache(const std::string& agent_id);
    
    /**
     * Get from distributed cache
     */
    template<typename T>
    bool get(const std::string& key, T& value);
    
    /**
     * Put into distributed cache
     */
    template<typename T>
    void put(const std::string& key, const T& value,
            std::chrono::milliseconds ttl = std::chrono::milliseconds(0));
    
    /**
     * Register cache for type
     */
    template<typename T>
    void registerCache(std::shared_ptr<CoherentCache<T>> cache);

private:
    std::string agent_id_;
    std::map<std::string, void*> caches_; // Type-erased cache pointers
    std::mutex mutex_;
};

// Template implementations
template<typename T>
CoherentCache<T>::CoherentCache(const std::string& agent_id)
    : agent_id_(agent_id)
{
}

template<typename T>
bool CoherentCache<T>::get(const std::string& key, T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return false;
    }
    
    auto& line = it->second;
    
    // Check expiration
    if (line.entry.isExpired()) {
        cache_.erase(it);
        return false;
    }
    
    // Handle coherence
    if (line.state == CoherenceState::INVALID) {
        requestShared(key);
    }
    
    if (line.state == CoherenceState::SHARED || 
        line.state == CoherenceState::EXCLUSIVE ||
        line.state == CoherenceState::MODIFIED) {
        value = line.entry.data;
        return true;
    }
    
    return false;
}

template<typename T>
void CoherentCache<T>::put(const std::string& key, const T& value,
                          std::chrono::milliseconds ttl) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& line = cache_[key];
    line.key = key;
    line.entry.data = value;
    line.entry.timestamp = std::chrono::system_clock::now();
    line.entry.ttl = ttl;
    line.entry.dirty = true;
    
    // Request exclusive access for write
    if (line.state != CoherenceState::EXCLUSIVE && 
        line.state != CoherenceState::MODIFIED) {
        requestExclusive(key);
    }
    
    line.state = CoherenceState::MODIFIED;
    line.owner = agent_id_;
}

template<typename T>
void CoherentCache<T>::invalidate(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        // Notify other agents
        if (coherence_callback_) {
            coherence_callback_("INVALIDATE", key, agent_id_);
        }
        cache_.erase(it);
    }
}

template<typename T>
void CoherentCache<T>::requestShared(const std::string& key) {
    if (coherence_callback_) {
        coherence_callback_("REQUEST_SHARED", key, agent_id_);
    }
    // Update state
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.state = CoherenceState::SHARED;
        it->second.sharers.push_back(agent_id_);
    }
}

template<typename T>
void CoherentCache<T>::requestExclusive(const std::string& key) {
    if (coherence_callback_) {
        coherence_callback_("REQUEST_EXCLUSIVE", key, agent_id_);
    }
    // Update state
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.state = CoherenceState::EXCLUSIVE;
        it->second.owner = agent_id_;
        it->second.sharers.clear();
    }
}

template<typename T>
void CoherentCache<T>::upgradeToModified(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it != cache_.end()) {
        it->second.state = CoherenceState::MODIFIED;
        it->second.entry.dirty = true;
    }
}

template<typename T>
void CoherentCache<T>::handleCoherenceMessage(const std::string& message_type,
                                             const std::string& key,
                                             const std::string& from_agent) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) return;
    
    auto& line = it->second;
    
    if (message_type == "INVALIDATE") {
        line.state = CoherenceState::INVALID;
    } else if (message_type == "REQUEST_SHARED") {
        if (line.state == CoherenceState::EXCLUSIVE) {
            line.state = CoherenceState::SHARED;
            line.sharers.push_back(from_agent);
        }
    }
}

template<typename T>
void CoherentCache<T>::setCoherenceCallback(
    std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    coherence_callback_ = callback;
}

} // namespace cache

#endif // CACHE_COHERENT_HPP

