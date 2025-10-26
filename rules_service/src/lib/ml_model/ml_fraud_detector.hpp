#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <transaction/transaction.pb.h>

// Forward declarations для XGBoost и LightGBM
typedef void* BoosterHandle;
typedef void* DMatrixHandle;

namespace fraud_detection {

// Статистика аккаунта для вычисления признаков
struct AccountStats {
    double time_since_last_transaction = 0.0;
    double spending_deviation_score = 0.0;
    double velocity_score = 0.0;
    double geo_anomaly_score = 1.0;
};

// Интерфейс для получения истории транзакций из БД/Redis
class TransactionHistoryProvider {
public:
    virtual ~TransactionHistoryProvider() = default;
    
    // Получить все транзакции для указанного аккаунта до заданного времени
    virtual std::vector<transaction::Transaction> GetAccountHistory(
        const std::string& account_id,
        int64_t before_timestamp) = 0;
};

// ML детектор мошенничества
class MLFraudDetector {
public:
    MLFraudDetector();
    ~MLFraudDetector();


    // Загрузить модель и признаки по uuid
    // config_dir/<uuid>.xgb (обязательно)
    // config_dir/<uuid>_columns.txt (обязательно)
    bool LoadModelByUuid(const std::string& config_dir, const std::string& uuid);

    // Предсказать вероятность мошенничества для транзакции
    // Возвращает значение от 0.0 до 1.0
    // provider - источник истории транзакций для вычисления признаков
    double PredictFraudProbability(
        const transaction::Transaction& txn,
        TransactionHistoryProvider& provider);

    // Проверка, загружены ли модели
    bool IsLoaded() const { return xgb_model_ != nullptr; }

    // Получить версию модели (путь к конфигу)
    std::string GetVersion() const { return config_dir_; }

private:
    // Вычислить признаки аккаунта на основе истории
    AccountStats ComputeAccountStats(
        const std::string& account_id,
        int64_t current_timestamp,
        double current_amount,
        const std::string& current_location,
        TransactionHistoryProvider& provider);

    // Создать вектор признаков для XGBoost
    std::vector<float> CreateFeatureVector(
        const transaction::Transaction& txn,
        const AccountStats& stats);

    // Парсинг timestamp из строки в epoch seconds
    int64_t ParseTimestamp(const std::string& timestamp_str);

    // Вспомогательная функция: безопасное преобразование в float
    static float SafeFloat(double value);

    // Модели
    BoosterHandle lgbm_model_ = nullptr;  // Опциональная LightGBM модель (stage 1)
    BoosterHandle xgb_model_ = nullptr;   // XGBoost модель (stage 2)
    
    // Конфигурация
    std::string config_dir_;
    std::vector<std::string> feature_names_;  // Список признаков из final_xgb_columns.txt
    std::unordered_map<std::string, int> feature_index_map_;  // Индексы признаков
};

} // namespace fraud_detection
