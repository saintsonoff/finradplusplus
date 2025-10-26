#include "ml_rule.hpp"
#include <userver/logging/log.hpp>
#include <stdexcept>

namespace fraud_detection {

MlRuleAnalyzer::MlRuleAnalyzer(const rules::RuleConfig& rule_config,
                               std::shared_ptr<MLFraudDetector> ml_detector,
                               std::shared_ptr<TransactionHistoryProvider> history_provider)
    : rule_config_(rule_config)
    , ml_detector_(std::move(ml_detector))
    , history_provider_(std::move(history_provider))
    , threshold_(0.5) {  // Порог по умолчанию
    
    // Извлечь порог из конфигурации ML правила
    if (rule_config_.has_ml_rule()) {
        threshold_ = rule_config_.ml_rule().lower_bound();
        LOG_INFO() << "ML Rule threshold (lower_bound) set to: " << threshold_;
    }
}

bool MlRuleAnalyzer::IsFraudTransaction(const transaction::Transaction& transaction) const {
    if (!ml_detector_) {
        LOG_ERROR() << "ML detector not initialized";
        return false;  // Консервативный подход: не блокировать если модель не загружена
    }
    
    if (!ml_detector_->IsLoaded()) {
        LOG_ERROR() << "ML model not loaded";
        return false;
    }
    
    if (!history_provider_) {
        LOG_ERROR() << "History provider not initialized";
        return false;
    }
    
    try {
        // Получить вероятность мошенничества от модели
        double fraud_probability = ml_detector_->PredictFraudProbability(transaction, *history_provider_);
        
        // Сравнить с порогом
        bool is_fraud = fraud_probability >= threshold_;
        
        LOG_INFO() << "Transaction " << transaction.transaction_id() 
                   << " fraud probability: " << fraud_probability 
                   << " (threshold: " << threshold_ << ") -> " 
                   << (is_fraud ? "FRAUD" : "LEGITIMATE");
        
        return is_fraud;
        
    } catch (const std::exception& ex) {
        LOG_ERROR() << "ML prediction failed: " << ex.what();
        return false;  // Консервативный подход при ошибке
    }
}

}
