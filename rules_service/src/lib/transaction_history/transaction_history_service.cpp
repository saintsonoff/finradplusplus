#include "transaction_history_service.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>

namespace fraud_detection {

TransactionHistoryService::TransactionHistoryService(
    userver::storages::postgres::ClusterPtr pg_cluster)
    : pg_cluster_(std::move(pg_cluster)) {}

void TransactionHistoryService::SaveTransaction(
    const transaction::Transaction& tx) {
    try {
        LOG_DEBUG() << "SaveTransaction: executing INSERT for transaction: " << tx.transaction_id()
                    << " account: " << tx.sender_account();

        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO transactions "
            "(transaction_id, sender_account, times_tamp, receiver_account, amount, "
            "transaction_type, merchant_category, location, device_used, payment_channel, "
            "ip_address, device_hash) "
            "VALUES ($1, $2, to_timestamp($3), $4, $5, $6::transaction_type, $7, $8, "
            "$9::device_used, $10::payment_channel, $11, $12) "
            "ON CONFLICT (transaction_id) DO NOTHING",
            tx.transaction_id(),
            tx.sender_account(),
            std::stoll(tx.timestamp()),
            tx.receiver_account(),
            tx.amount(),
            TransactionTypeToString(tx.transaction_type()),
            tx.merchant_category(),
            tx.location(),
            DeviceUsedToString(tx.device_used()),
            PaymentChannelToString(tx.payment_channel()),
            tx.ip_address(),
            tx.device_hash()
        );
        
        LOG_INFO() << "Saved transaction " << tx.transaction_id() 
                   << " to PostgreSQL for account " << tx.sender_account();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to save transaction to PostgreSQL: " << e.what();
    }
}

std::vector<transaction::Transaction> 
TransactionHistoryService::GetAccountHistory(
    const std::string& account_id, 
    int limit) const {
    std::vector<transaction::Transaction> history;
    try {
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT transaction_id, sender_account, EXTRACT(EPOCH FROM times_tamp)::bigint as timestamp, "
            "receiver_account, amount, transaction_type::text, merchant_category, location, "
            "device_used::text, payment_channel::text, ip_address, device_hash "
            "FROM transactions "
            "WHERE sender_account = $1 "
            "ORDER BY times_tamp DESC "
            "LIMIT $2",
            account_id,
            limit
        );
        
        for (const auto& row : result) {
            transaction::Transaction tx;
            tx.set_transaction_id(row["transaction_id"].As<std::string>());
            tx.set_sender_account(row["sender_account"].As<std::string>());
            tx.set_timestamp(std::to_string(row["timestamp"].As<int64_t>()));
            tx.set_receiver_account(row["receiver_account"].As<std::string>());
            tx.set_amount(row["amount"].As<double>());
            tx.set_transaction_type(StringToTransactionType(row["transaction_type"].As<std::string>()));
            tx.set_merchant_category(row["merchant_category"].As<std::string>());
            tx.set_location(row["location"].As<std::string>());
            tx.set_device_used(StringToDeviceUsed(row["device_used"].As<std::string>()));
            tx.set_payment_channel(StringToPaymentChannel(row["payment_channel"].As<std::string>()));
            tx.set_ip_address(row["ip_address"].As<std::string>());
            tx.set_device_hash(row["device_hash"].As<std::string>());
            
            history.push_back(std::move(tx));
        }
        
        LOG_INFO() << "Retrieved " << history.size() 
                   << " transactions for account " << account_id;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to get transaction history from PostgreSQL: " 
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
        auto result = pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kSlave,
            "SELECT transaction_id, sender_account, EXTRACT(EPOCH FROM times_tamp)::bigint as timestamp, "
            "receiver_account, amount, transaction_type::text, merchant_category, location, "
            "device_used::text, payment_channel::text, ip_address, device_hash "
            "FROM transactions "
            "WHERE sender_account = $1 "
            "AND times_tamp >= NOW() - INTERVAL '1 minute' * $2 "
            "ORDER BY times_tamp DESC "
            "LIMIT $3",
            account_id,
            minutes,
            limit
        );
        
        for (const auto& row : result) {
            transaction::Transaction tx;
            tx.set_transaction_id(row["transaction_id"].As<std::string>());
            tx.set_sender_account(row["sender_account"].As<std::string>());
            tx.set_timestamp(std::to_string(row["timestamp"].As<int64_t>()));
            tx.set_receiver_account(row["receiver_account"].As<std::string>());
            tx.set_amount(row["amount"].As<double>());
            tx.set_transaction_type(StringToTransactionType(row["transaction_type"].As<std::string>()));
            tx.set_merchant_category(row["merchant_category"].As<std::string>());
            tx.set_location(row["location"].As<std::string>());
            tx.set_device_used(StringToDeviceUsed(row["device_used"].As<std::string>()));
            tx.set_payment_channel(StringToPaymentChannel(row["payment_channel"].As<std::string>()));
            tx.set_ip_address(row["ip_address"].As<std::string>());
            tx.set_device_hash(row["device_hash"].As<std::string>());
            
            recent.push_back(std::move(tx));
        }
        
        LOG_INFO() << "Retrieved " << recent.size() 
                   << " recent transactions (last " << minutes 
                   << " minutes) for account " << account_id;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to get recent transactions from PostgreSQL: " 
                   << e.what();
    }
    return recent;
}

float TransactionHistoryService::ExecuteAggregateQuery(const std::string& sql, const std::string& param) const {
    try {
        LOG_DEBUG() << "ExecuteAggregateQuery SQL: " << sql;
        LOG_DEBUG() << "  param[1] = " << param;

        auto result = pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster, sql, param);
        if (result.IsEmpty()) {
            LOG_DEBUG() << "ExecuteAggregateQuery: result set is empty";
            return 0.0f;
        }

        // Если результат NULL (например, SUM/AVG/MIN/MAX по пустому набору), возвращаем 0.0f
        if (result[0][0].IsNull()) {
            LOG_DEBUG() << "ExecuteAggregateQuery: aggregate result is NULL (empty set), returning 0.0f";
            return 0.0f;
        }

        // Для COUNT всегда int64, для SUM/AVG/… может быть double/float/int64
        try {
            int64_t ival = result[0][0].As<int64_t>();
            LOG_DEBUG() << "ExecuteAggregateQuery: raw int64 result = " << ival;
            return static_cast<float>(ival);
        } catch (...) {}
        try {
            double dval = result[0][0].As<double>();
            LOG_DEBUG() << "ExecuteAggregateQuery: raw double result = " << dval;
            return static_cast<float>(dval);
        } catch (...) {}
        try {
            float fval = result[0][0].As<float>();
            LOG_DEBUG() << "ExecuteAggregateQuery: raw float result = " << fval;
            return fval;
        } catch (...) {}

        LOG_DEBUG() << "ExecuteAggregateQuery: unknown type for aggregate column, returning 0.0f";
        return 0.0f;
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to execute aggregate SQL: " << e.what();
        return 0.0f;
    }
}

// Enum conversion helpers
std::string TransactionHistoryService::TransactionTypeToString(
    transaction::Transaction::TransactionType type) const {
    switch (type) {
        case transaction::Transaction::WITHDRAWAL: return "WITHDRAWAL";
        case transaction::Transaction::DEPOSIT: return "DEPOSIT";
        case transaction::Transaction::TRANSFER: return "TRANSFER";
        case transaction::Transaction::PAYMENT: return "PAYMENT";
        default: return "PAYMENT";
    }
}

std::string TransactionHistoryService::DeviceUsedToString(
    transaction::Transaction::DeviceUsed device) const {
    switch (device) {
        case transaction::Transaction::MOBILE: return "MOBILE";
        case transaction::Transaction::ATM: return "ATM";
        case transaction::Transaction::POS: return "POS";
        case transaction::Transaction::WEB: return "WEB";
        default: return "WEB";
    }
}

std::string TransactionHistoryService::PaymentChannelToString(
    transaction::Transaction::PaymentChannel channel) const {
    switch (channel) {
        case transaction::Transaction::UPI: return "UPI";
        case transaction::Transaction::CARD: return "CARD";
        case transaction::Transaction::ACH: return "ACH";
        case transaction::Transaction::WIRE_TRANSFER: return "WIRE_TRANSFER";
        default: return "CARD";
    }
}

transaction::Transaction::TransactionType TransactionHistoryService::StringToTransactionType(
    const std::string& str) const {
    if (str == "WITHDRAWAL") return transaction::Transaction::WITHDRAWAL;
    if (str == "DEPOSIT") return transaction::Transaction::DEPOSIT;
    if (str == "TRANSFER") return transaction::Transaction::TRANSFER;
    if (str == "PAYMENT") return transaction::Transaction::PAYMENT;
    return transaction::Transaction::PAYMENT;
}

transaction::Transaction::DeviceUsed TransactionHistoryService::StringToDeviceUsed(
    const std::string& str) const {
    if (str == "MOBILE") return transaction::Transaction::MOBILE;
    if (str == "ATM") return transaction::Transaction::ATM;
    if (str == "POS") return transaction::Transaction::POS;
    if (str == "WEB") return transaction::Transaction::WEB;
    return transaction::Transaction::WEB;
}

transaction::Transaction::PaymentChannel TransactionHistoryService::StringToPaymentChannel(
    const std::string& str) const {
    if (str == "UPI") return transaction::Transaction::UPI;
    if (str == "CARD") return transaction::Transaction::CARD;
    if (str == "ACH") return transaction::Transaction::ACH;
    if (str == "WIRE_TRANSFER") return transaction::Transaction::WIRE_TRANSFER;
    return transaction::Transaction::CARD;
}

}  // namespace fraud_detection
