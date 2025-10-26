#pragma once

#include "ml_fraud_detector.hpp"
#include "../transaction_history/transaction_history_service.hpp"

namespace fraud_detection {

class RedisHistoryProvider : public TransactionHistoryProvider {
public:
    explicit RedisHistoryProvider(std::shared_ptr<TransactionHistoryService> history_service)
        : history_service_(std::move(history_service)) {}
    
    ~RedisHistoryProvider() override = default;
    
    std::vector<transaction::Transaction> GetAccountHistory(
        const std::string& account_id,
        int64_t before_timestamp) override;

private:
    std::shared_ptr<TransactionHistoryService> history_service_;
};

} // namespace fraud_detection
