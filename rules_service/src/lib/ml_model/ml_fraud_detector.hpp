#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <transaction/transaction.pb.h>

typedef void* BoosterHandle;
typedef void* DMatrixHandle;

namespace fraud_detection {

struct AccountStats {
    double time_since_last_transaction = 0.0;
    double spending_deviation_score = 0.0;
    double velocity_score = 0.0;
    double geo_anomaly_score = 1.0;
};

class TransactionHistoryProvider {
public:
    virtual ~TransactionHistoryProvider() = default;
    
    virtual std::vector<transaction::Transaction> GetAccountHistory(
        const std::string& account_id,
        int64_t before_timestamp) = 0;
};

class MLFraudDetector {
public:
    MLFraudDetector();
    ~MLFraudDetector();

    bool LoadModelByUuid(const std::string& config_dir, const std::string& uuid);

    double PredictFraudProbability(
        const transaction::Transaction& txn,
        TransactionHistoryProvider& provider);


    bool IsLoaded() const { return xgb_model_ != nullptr; }


    std::string GetVersion() const { return config_dir_; }

private:

    AccountStats ComputeAccountStats(
        const std::string& account_id,
        int64_t current_timestamp,
        double current_amount,
        const std::string& current_location,
        TransactionHistoryProvider& provider);

    std::vector<float> CreateFeatureVector(
        const transaction::Transaction& txn,
        const AccountStats& stats);


    int64_t ParseTimestamp(const std::string& timestamp_str);


    static float SafeFloat(double value);


    BoosterHandle lgbm_model_ = nullptr;
    BoosterHandle xgb_model_ = nullptr;
    
    
    std::string config_dir_;
    std::vector<std::string> feature_names_;
    std::unordered_map<std::string, int> feature_index_map_;
};

} // namespace fraud_detection
