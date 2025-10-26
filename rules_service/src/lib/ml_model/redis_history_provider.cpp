#include "redis_history_provider.hpp"
#include <userver/logging/log.hpp>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace fraud_detection {

std::vector<transaction::Transaction> RedisHistoryProvider::GetAccountHistory(
    const std::string& account_id,
    int64_t before_timestamp) {
    
    if (!history_service_) {
        LOG_WARNING() << "TransactionHistoryService not available";
        return {};
    }
    
    // Получить все транзакции аккаунта из Redis
    // TransactionHistoryService::GetAccountHistory возвращает последние N транзакций
    auto all_transactions = history_service_->GetAccountHistory(account_id, 1000);
    
    // Фильтровать только транзакции до указанного времени
    std::vector<transaction::Transaction> filtered;
    filtered.reserve(all_transactions.size());
    
    for (const auto& txn : all_transactions) {
        // Парсинг timestamp (должен быть в формате ISO или unix timestamp)
        int64_t txn_timestamp = 0;
        const std::string& ts_str = txn.timestamp();
        
        if (!ts_str.empty()) {
            if (ts_str.find('T') != std::string::npos) {
                // ISO формат - парсим через strptime
                std::tm tm{};
                std::istringstream ss(ts_str);
                ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
                if (!ss.fail()) {
#if defined(_WIN32)
                    txn_timestamp = static_cast<int64_t>(_mkgmtime(&tm));
#else
                    txn_timestamp = static_cast<int64_t>(timegm(&tm));
#endif
                }
            } else {
                // Unix timestamp
                try {
                    txn_timestamp = std::stoll(ts_str);
                } catch (...) {
                    LOG_WARNING() << "Failed to parse timestamp: " << ts_str;
                    continue;
                }
            }
        }
        
        // Добавить только если timestamp < before_timestamp
        if (txn_timestamp < before_timestamp) {
            filtered.push_back(txn);
        }
    }
    
    LOG_DEBUG() << "Filtered " << filtered.size() << " transactions for account " 
                << account_id << " before timestamp " << before_timestamp;
    
    return filtered;
}

} // namespace fraud_detection
