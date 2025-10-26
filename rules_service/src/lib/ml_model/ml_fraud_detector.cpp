#include "ml_fraud_detector.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <stdexcept>

#ifdef HAVE_LIGHTGBM
#include <LightGBM/c_api.h>
#endif

#include <xgboost/c_api.h>

#include <userver/logging/log.hpp>

namespace fraud_detection {

namespace {

std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::time_t ParseIsoToEpochSeconds(const std::string& iso) {
    std::string s = iso;
    size_t dot = s.find('.');
    std::string core = (dot == std::string::npos) ? s : s.substr(0, dot);
    std::tm tm{};
    std::istringstream ss(core);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (!ss.fail()) {
#if defined(_WIN32)
        return _mkgmtime(&tm);
#else
        return timegm(&tm);
#endif
    }
    try {
        return static_cast<std::time_t>(std::stoll(iso));
    } catch (...) {
        return 0;
    }
}

} // anonymous namespace

MLFraudDetector::MLFraudDetector() = default;

MLFraudDetector::~MLFraudDetector() {
#ifdef HAVE_LIGHTGBM
    if (lgbm_model_) {
        LGBM_BoosterFree(lgbm_model_);
    }
#endif
    if (xgb_model_) {
        XGBoosterFree(xgb_model_);
    }
}


bool MLFraudDetector::LoadModelByUuid(const std::string& config_dir, const std::string& uuid) {
    config_dir_ = config_dir;
    feature_names_.clear();
    feature_index_map_.clear();
    if (xgb_model_) {
        XGBoosterFree(xgb_model_);
        xgb_model_ = nullptr;
    }
#ifdef HAVE_LIGHTGBM
    if (lgbm_model_) {
        LGBM_BoosterFree(lgbm_model_);
        lgbm_model_ = nullptr;
    }
#endif

    std::string columns_path = config_dir + "/" + uuid + "_columns.txt";
    std::ifstream columns_file(columns_path);
    if (!columns_file.is_open()) {
        LOG_ERROR() << "Cannot open feature columns file: " << columns_path;
        return false;
    }
    std::string line;
    int index = 0;
    while (std::getline(columns_file, line)) {
        line = Trim(line);
        if (!line.empty()) {
            feature_names_.push_back(line);
            feature_index_map_[line] = index++;
        }
    }
    columns_file.close();
    if (feature_names_.empty()) {
        LOG_ERROR() << "No features found in " << columns_path;
        return false;
    }
    LOG_INFO() << "Loaded " << feature_names_.size() << " features for uuid " << uuid;

#ifdef HAVE_LIGHTGBM
    std::string lgbm_path = config_dir + "/" + uuid + "_lgbm.txt";
    std::ifstream lgbm_check(lgbm_path);
    if (lgbm_check.good()) {
        lgbm_check.close();
        int num_iter = 0;
        if (LGBM_BoosterCreateFromModelfile(lgbm_path.c_str(), &num_iter, &lgbm_model_) == 0) {
            LOG_INFO() << "Loaded LightGBM model from " << lgbm_path << " for uuid " << uuid;
        } else {
            LOG_WARNING() << "Failed to load LightGBM model from " << lgbm_path << " for uuid " << uuid;
        }
    } else {
        LOG_INFO() << "LightGBM model not found (optional) for uuid " << uuid << ", skipping";
    }
#endif

    std::string xgb_path = config_dir + "/" + uuid + "_json.json";
    std::ifstream xgb_check(xgb_path);
    if (!xgb_check.good()) {
        LOG_ERROR() << "XGBoost model file not found: " << xgb_path;
        return false;
    }
    xgb_check.close();
    if (XGBoosterCreate(nullptr, 0, &xgb_model_) != 0) {
        LOG_ERROR() << "XGBoosterCreate failed";
        return false;
    }
    if (XGBoosterLoadModel(xgb_model_, xgb_path.c_str()) != 0) {
        LOG_ERROR() << "XGBoosterLoadModel failed for " << xgb_path;
        XGBoosterFree(xgb_model_);
        xgb_model_ = nullptr;
        return false;
    }
    LOG_INFO() << "Loaded XGBoost model from " << xgb_path << " for uuid " << uuid;
    return true;
}

int64_t MLFraudDetector::ParseTimestamp(const std::string& timestamp_str) {
    std::string t = Trim(timestamp_str);
    if (t.empty()) return 0;
    
    if (t.find('T') != std::string::npos) {
        return static_cast<int64_t>(ParseIsoToEpochSeconds(t));
    }
    
    try {
        return std::stoll(t);
    } catch (...) {
        return static_cast<int64_t>(ParseIsoToEpochSeconds(t));
    }
}

AccountStats MLFraudDetector::ComputeAccountStats(
    const std::string& account_id,
    int64_t current_ts,
    double current_amount,
    const std::string& current_location,
    TransactionHistoryProvider& provider) {
    
    AccountStats out;
    
    auto history = provider.GetAccountHistory(account_id, current_ts);
    
    if (history.empty()) {
        LOG_DEBUG() << "No history for account " << account_id;
        return out;
    }
    
    int64_t n = 0;
    double mean = 0.0, m2 = 0.0;
    int64_t last_before = 0;
    int64_t cnt_window = 0;
    std::unordered_map<std::string, int64_t> loc_counts;
    int64_t total_count = 0;
    const int64_t window_start = current_ts - 86400;
    
    for (const auto& txn : history) {
        int64_t ts = ParseTimestamp(txn.timestamp());
        double amt = txn.amount();
        
        double amt_log = std::log1p(std::max(0.0, static_cast<double>(amt)));
        
        n += 1;
        double delta = amt_log - mean;
        mean += delta / static_cast<double>(n);
        m2 += delta * (amt_log - mean);
        
        if (ts < current_ts && ts > last_before) {
            last_before = ts;
        }
        
        if (ts >= window_start && ts < current_ts) {
            cnt_window++;
        }
        
        std::string loc = txn.location();
        loc_counts[loc] += 1;
        total_count++;
    }
    
    out.time_since_last_transaction = (last_before > 0) 
        ? static_cast<double>(current_ts - last_before) 
        : 0.0;
    
    double current_amt_log = std::log1p(std::max(0.0, current_amount));
    double stddev = 0.0;
    if (n > 0) {
        double var = (m2 / static_cast<double>(n));
        if (var > 0.0) stddev = std::sqrt(var);
    }
    out.spending_deviation_score = (stddev > 1e-12) 
        ? ((current_amt_log - mean) / stddev) 
        : 0.0;
    
    out.velocity_score = static_cast<double>(cnt_window);
    
    int64_t loc_cnt = 0;
    auto it = loc_counts.find(current_location);
    if (it != loc_counts.end()) {
        loc_cnt = it->second;
    }
    
    if (total_count <= 0) {
        out.geo_anomaly_score = 1.0;
    } else {
        double frac = static_cast<double>(loc_cnt) / static_cast<double>(total_count);
        out.geo_anomaly_score = 1.0 - frac;
        out.geo_anomaly_score = std::max(0.0, std::min(1.0, out.geo_anomaly_score));
    }
    
    return out;
}

float MLFraudDetector::SafeFloat(double v) {
    if (!std::isfinite(v)) return 0.0f;
    const double MAXF = 3.4e37;
    if (v > MAXF) return static_cast<float>(MAXF);
    if (v < -MAXF) return static_cast<float>(-MAXF);
    return static_cast<float>(v);
}

std::vector<float> MLFraudDetector::CreateFeatureVector(
    const transaction::Transaction& txn,
    const AccountStats& stats) {
    
    std::vector<float> vec(feature_names_.size(), 0.0f);
    
    auto set_feature = [&](const std::string& name, double value) {
        auto it = feature_index_map_.find(name);
        if (it != feature_index_map_.end()) {
            vec[it->second] = SafeFloat(value);
        }
    };
    
    double amount_trans = std::log1p(std::max(0.0, static_cast<double>(txn.amount())));
    set_feature("amount", amount_trans);
    set_feature("time_since_last_transaction", stats.time_since_last_transaction);
    set_feature("spending_deviation_score", stats.spending_deviation_score);
    set_feature("velocity_score", stats.velocity_score);
    set_feature("geo_anomaly_score", stats.geo_anomaly_score);
    
    int64_t timestamp = ParseTimestamp(txn.timestamp());
    std::time_t t = static_cast<std::time_t>(timestamp);
    std::tm tm_utc;
#if defined(_WIN32)
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    
    int hour = tm_utc.tm_hour;
    int dayofweek = (tm_utc.tm_wday + 6) % 7;
    set_feature("hour_of_day", static_cast<double>(hour));
    set_feature("day_of_week", static_cast<double>(dayofweek));
    
    auto set_categorical = [&](const std::string& prefix, const std::string& value) {
        std::string feature_name = prefix + (value.empty() ? "nan" : value);
        auto it = feature_index_map_.find(feature_name);
        if (it != feature_index_map_.end()) {
            vec[it->second] = 1.0f;
        } else {
            std::string nan_name = prefix + "nan";
            auto it_nan = feature_index_map_.find(nan_name);
            if (it_nan != feature_index_map_.end()) {
                vec[it_nan->second] = 1.0f;
            }
        }
    };
    
    std::string transaction_type_str;
    switch (txn.transaction_type()) {
        case transaction::Transaction::DEPOSIT:
            transaction_type_str = "deposit";
            break;
        case transaction::Transaction::PAYMENT:
            transaction_type_str = "payment";
            break;
        case transaction::Transaction::TRANSFER:
            transaction_type_str = "transfer";
            break;
        case transaction::Transaction::WITHDRAWAL:
            transaction_type_str = "withdrawal";
            break;
        default:
            transaction_type_str = "";
            break;
    }
    
    set_categorical("transaction_type_", transaction_type_str);
    set_categorical("merchant_category_", txn.merchant_category());
    set_categorical("location_", txn.location());
    
    std::string device_str;
    switch (txn.device_used()) {
        case transaction::Transaction::ATM:
            device_str = "atm";
            break;
        case transaction::Transaction::MOBILE:
            device_str = "mobile";
            break;
        case transaction::Transaction::POS:
            device_str = "pos";
            break;
        case transaction::Transaction::WEB:
            device_str = "web";
            break;
        default:
            device_str = "";
            break;
    }
    set_categorical("device_used_", device_str);
    
    std::string channel_str;
    switch (txn.payment_channel()) {
        case transaction::Transaction::ACH:
            channel_str = "ACH";
            break;
        case transaction::Transaction::UPI:
            channel_str = "UPI";
            break;
        case transaction::Transaction::CARD:
            channel_str = "card";
            break;
        case transaction::Transaction::WIRE_TRANSFER:
            channel_str = "wire_transfer";
            break;
        default:
            channel_str = "";
            break;
    }
    set_categorical("payment_channel_", channel_str);
    
    return vec;
}

double MLFraudDetector::PredictFraudProbability(
    const transaction::Transaction& txn,
    TransactionHistoryProvider& provider) {
    
    if (!xgb_model_) {
        throw std::runtime_error("XGBoost model not loaded");
    }
    
    std::string sender = txn.sender_account();
    int64_t timestamp = ParseTimestamp(txn.timestamp());
    double amount = static_cast<double>(txn.amount());
    std::string location = txn.location();
    
    AccountStats stats = ComputeAccountStats(sender, timestamp, amount, location, provider);
    
#ifdef HAVE_LIGHTGBM
    if (lgbm_model_) {
        std::vector<double> lgbm_feats;
        lgbm_feats.push_back(std::log1p(std::max(0.0, amount)));
        lgbm_feats.push_back(stats.time_since_last_transaction);
        lgbm_feats.push_back(stats.spending_deviation_score);
        lgbm_feats.push_back(stats.velocity_score);
        lgbm_feats.push_back(stats.geo_anomaly_score);
        
        for (int i = 0; i < 5; ++i) lgbm_feats.push_back(0.0);
        
        std::time_t t = static_cast<std::time_t>(timestamp);
        std::tm tm_utc;
#if defined(_WIN32)
        gmtime_s(&tm_utc, &t);
#else
        gmtime_r(&t, &tm_utc);
#endif
        lgbm_feats.push_back(static_cast<double>(tm_utc.tm_hour));
        lgbm_feats.push_back(static_cast<double>((tm_utc.tm_wday + 6) % 7));
        
        double lgbm_score = 0.0;
        int64_t out_len = 0;
        if (LGBM_BoosterPredictForMat(lgbm_model_, lgbm_feats.data(), C_API_DTYPE_FLOAT64, 
                                     1, static_cast<int>(lgbm_feats.size()), 1,
                                     C_API_PREDICT_NORMAL, 0, -1, "", &out_len, &lgbm_score) == 0) {
            LOG_DEBUG() << "LightGBM stage1 score: " << lgbm_score;
        }
    }
#endif
    
    std::vector<float> xgb_input = CreateFeatureVector(txn, stats);
    
    DMatrixHandle dmat;
    if (XGDMatrixCreateFromMat(xgb_input.data(), 1, 
                               static_cast<bst_ulong>(xgb_input.size()), 
                               0.0f, &dmat) != 0) {
        throw std::runtime_error("XGDMatrixCreateFromMat failed");
    }
    
    bst_ulong out_len = 0;
    const float* out_result = nullptr;
    if (XGBoosterPredict(xgb_model_, dmat, 0, 0, 0, &out_len, &out_result) != 0) {
        XGDMatrixFree(dmat);
        throw std::runtime_error("XGBoosterPredict failed");
    }
    
    float score = out_result[0];
    XGDMatrixFree(dmat);
    
    LOG_INFO() << "XGBoost fraud probability for txn " << txn.transaction_id() 
               << ": " << score;
    
    return static_cast<double>(score);
}

} // namespace fraud_detection
