#pragma once

#include "rule_interface/IRule.hpp"
#include "ml_model/ml_fraud_detector.hpp"
#include "ml_model/redis_history_provider.hpp"
#include <rules/rule_config.pb.h>
#include <memory>

namespace fraud_detection {

class MlRuleAnalyzer : public IRule {
public:
    MlRuleAnalyzer(const rules::RuleConfig& rule_config,
                   std::shared_ptr<MLFraudDetector> ml_detector,
                   std::shared_ptr<TransactionHistoryProvider> history_provider);

    bool IsFraudTransaction(const transaction::Transaction& transaction) const override;

private:
    const rules::RuleConfig rule_config_;
    std::shared_ptr<MLFraudDetector> ml_detector_;
    std::shared_ptr<TransactionHistoryProvider> history_provider_;
    double threshold_;
};

} // namespace fraud_detection
