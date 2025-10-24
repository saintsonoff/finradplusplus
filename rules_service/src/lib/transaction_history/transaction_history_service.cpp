#include "transaction_history_service.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/redis/command_control.hpp>

namespace fraud_detection {

TransactionHistoryService::TransactionHistoryService(
    userver::storages::redis::ClientPtr redis_client)
    : redis_client_(std::move(redis_client)) {}

void TransactionHistoryService::SaveTransaction(
    const transaction::Transaction& tx) {
    try {
        std::string key = MakeAccountKey(tx.sender_account());
        std::string serialized;
        if (!tx.SerializeToString(&serialized)) {
            LOG_ERROR() << "Failed to serialize transaction " << tx.transaction_id();
            return;
        }
        double score = std::stod(tx.timestamp());
        redis_client_->Zadd(key, score, serialized, {}).Get();
        redis_client_->Expire(key, kTransactionTTL, {}).Get();
        LOG_DEBUG() << "Saved transaction " << tx.transaction_id() 
                   << " to Redis history for account " << tx.sender_account();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to save transaction to Redis: " << e.what();
    }
}

std::vector<transaction::Transaction> 
TransactionHistoryService::GetAccountHistory(
    const std::string& account_id, 
    int limit) const {
    std::vector<transaction::Transaction> history;
    try {
        std::string key = MakeAccountKey(account_id);
        auto request = redis_client_->Zrangebyscore(key, "-inf", "+inf", {});
        auto result = request.Get();
        if (result.empty()) {
            return history;
        }
        const auto& all_txs = result;
        size_t start_idx = all_txs.size() > static_cast<size_t>(limit) ? 
                           all_txs.size() - limit : 0;
        for (size_t i = all_txs.size(); i > start_idx; --i) {
            const auto& serialized = all_txs[i - 1];
            transaction::Transaction tx;
            if (tx.ParseFromString(serialized)) {
                history.push_back(std::move(tx));
            } else {
                LOG_WARNING() << "Failed to deserialize transaction from Redis";
            }
        }
        LOG_DEBUG() << "Retrieved " << history.size() 
                   << " transactions for account " << account_id;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to get transaction history from Redis: " 
                   << e.what();
    }
    return history;
}

std::vector<transaction::Transaction> 
TransactionHistoryService::GetRecentTransactions(
    const std::string& account_id,
    int minutes,
    int limit) const {
    std::vector<transaction::Transaction> recent;
    try {
        std::string key = MakeAccountKey(account_id);
        auto now = std::chrono::system_clock::now();
        auto cutoff = now - std::chrono::minutes(minutes);
        auto cutoff_timestamp = std::chrono::duration_cast<
            std::chrono::seconds>(cutoff.time_since_epoch()).count();
        auto request = redis_client_->Zrangebyscore(
            key,
            std::to_string(cutoff_timestamp),
            "+inf",
            {}
        );
        auto result = request.Get();
        if (result.empty()) {
            return recent;
        }
        const auto& all_txs = result;
        size_t start_idx = all_txs.size() > static_cast<size_t>(limit) ? 
                           all_txs.size() - limit : 0;
        for (size_t i = all_txs.size(); i > start_idx; --i) {
            const auto& serialized = all_txs[i - 1];
            transaction::Transaction tx;
            if (tx.ParseFromString(serialized)) {
                recent.push_back(std::move(tx));
            }
        }
        LOG_DEBUG() << "Retrieved " << recent.size() 
                   << " recent transactions (last " << minutes 
                   << " minutes) for account " << account_id;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to get recent transactions from Redis: " 
                   << e.what();
    }
    return recent;
}

std::string TransactionHistoryService::MakeAccountKey(
    const std::string& account_id) const {
    return "tx_history:" + account_id;
}

}  // namespace fraud_detection
