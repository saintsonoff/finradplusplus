#include "ml_rule.hpp"

namespace fraud_detection {

MlRuleAnalyzer::MlRuleAnalyzer(const rules::RuleConfig& rule_config)
    : rule_config_(rule_config) {}

bool MlRuleAnalyzer::IsFraudTransaction(const transaction::Transaction& transaction) const {
    return true;
}

}
