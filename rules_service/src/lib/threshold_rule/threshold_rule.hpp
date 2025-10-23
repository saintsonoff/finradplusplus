#pragma once

#include "rule_interface/IRule.hpp"
#include "rule_config.pb.h"
#include "pattern_rule.pb.h"

class ThresholdRuleAnalyzer : public IRule {
public:
    ThresholdRuleAnalyzer(const rules::RuleConfig& rule_config)
    : rule_config_(rule_config) {}

    bool IsFraudTransaction(const transaction::Transaction& transaction) const {
        return true;
    }

private:
    const rules::RuleConfig rule_config_;
};
