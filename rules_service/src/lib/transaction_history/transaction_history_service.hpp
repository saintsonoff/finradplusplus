#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <userver/storages/redis/client.hpp>
#include <transaction/transaction.pb.h>

namespace fraud_detection {

class TransactionHistoryService {
public:
    explicit TransactionHistoryService(
        userver::storages::redis::ClientPtr redis_client);
    virtual ~TransactionHistoryService() = default;
    virtual void SaveTransaction(const transaction::Transaction& tx);
    virtual std::vector<transaction::Transaction> GetAccountHistory(
        const std::string& account_id, 
        int limit = 100) const;
    virtual std::vector<transaction::Transaction> GetRecentTransactions(
        const std::string& account_id,
        int minutes,
        int limit = 100) const;

private:
    std::string MakeAccountKey(const std::string& account_id) const;
    userver::storages::redis::ClientPtr redis_client_;
    static constexpr std::chrono::seconds kTransactionTTL{7 * 24 * 3600};
};

}
