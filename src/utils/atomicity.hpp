/*
 * Copyright (C) 2025, Shyamal Suhana Chandra
 * No MIT License
 * 
 * Atomicity: Transaction support, rollback capability
 */

#ifndef ATOMICITY_HPP
#define ATOMICITY_HPP

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <map>

namespace atomicity {

/**
 * Transaction state
 */
enum class TransactionState {
    PENDING,
    COMMITTED,
    ROLLED_BACK,
    ABORTED
};

/**
 * Operation in transaction
 */
class TransactionOperation {
public:
    virtual ~TransactionOperation() = default;
    
    /**
     * Execute operation
     */
    virtual bool execute() = 0;
    
    /**
     * Rollback operation
     */
    virtual bool rollback() = 0;
    
    /**
     * Get operation description
     */
    virtual std::string getDescription() const = 0;
};

/**
 * Transaction manager
 */
class Transaction {
public:
    Transaction(const std::string& transaction_id);
    ~Transaction();
    
    /**
     * Add operation to transaction
     */
    void addOperation(std::shared_ptr<TransactionOperation> operation);
    
    /**
     * Commit transaction
     */
    bool commit();
    
    /**
     * Rollback transaction
     */
    bool rollback();
    
    /**
     * Get transaction ID
     */
    const std::string& getId() const { return transaction_id_; }
    
    /**
     * Get state
     */
    TransactionState getState() const { return state_; }

private:
    std::string transaction_id_;
    std::vector<std::shared_ptr<TransactionOperation>> operations_;
    TransactionState state_;
    std::mutex mutex_;
    
    bool executeAll();
    bool rollbackAll();
};

/**
 * Transaction manager
 */
class TransactionManager {
public:
    TransactionManager();
    
    /**
     * Begin transaction
     */
    std::shared_ptr<Transaction> beginTransaction();
    
    /**
     * Get transaction
     */
    std::shared_ptr<Transaction> getTransaction(const std::string& transaction_id);
    
    /**
     * Commit transaction
     */
    bool commitTransaction(const std::string& transaction_id);
    
    /**
     * Rollback transaction
     */
    bool rollbackTransaction(const std::string& transaction_id);

private:
    std::map<std::string, std::shared_ptr<Transaction>> transactions_;
    std::atomic<uint64_t> transaction_counter_;
    std::mutex mutex_;
    
    std::string generateTransactionId();
};

/**
 * Two-phase commit coordinator
 */
class TwoPhaseCommitCoordinator {
public:
    TwoPhaseCommitCoordinator();
    
    /**
     * Begin two-phase commit
     */
    bool beginCommit(const std::string& transaction_id,
                    const std::vector<std::string>& participants);
    
    /**
     * Vote on commit
     */
    bool vote(const std::string& transaction_id, bool can_commit);
    
    /**
     * Complete commit
     */
    bool completeCommit(const std::string& transaction_id);

private:
    struct CommitState {
        std::vector<std::string> participants;
        std::map<std::string, bool> votes;
        bool committed;
    };
    
    std::map<std::string, CommitState> commits_;
    std::mutex mutex_;
};

} // namespace atomicity

#endif // ATOMICITY_HPP

