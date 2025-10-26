#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <userver/storages/postgres/cluster.hpp>
#include <transaction/transaction.pb.h>

namespace fraud_detection {

class TransactionHistoryService {
public:
    explicit TransactionHistoryService(
        userver::storages::postgres::ClusterPtr pg_cluster);
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
    std::string TransactionTypeToString(transaction::Transaction::TransactionType type) const;
    std::string DeviceUsedToString(transaction::Transaction::DeviceUsed device) const;
    std::string PaymentChannelToString(transaction::Transaction::PaymentChannel channel) const;
    
    transaction::Transaction::TransactionType StringToTransactionType(const std::string& str) const;
    transaction::Transaction::DeviceUsed StringToDeviceUsed(const std::string& str) const;
    transaction::Transaction::PaymentChannel StringToPaymentChannel(const std::string& str) const;
    
    userver::storages::postgres::ClusterPtr pg_cluster_;
};

}
