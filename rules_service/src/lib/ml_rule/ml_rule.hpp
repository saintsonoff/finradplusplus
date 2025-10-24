#pragma once

#include "rule_interface/IRule.hpp"
#include <rules/rule_config.pb.h>

namespace fraud_detection {

class MlRuleAnalyzer : public IRule {
public:
    explicit MlRuleAnalyzer(const rules::RuleConfig& rule_config);

    bool IsFraudTransaction(const transaction::Transaction& transaction) const override;

private:
    const rules::RuleConfig rule_config_;
};

}
