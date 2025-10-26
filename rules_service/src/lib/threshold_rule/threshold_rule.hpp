#pragma once

#include "rule_interface/IRule.hpp"
#include "rule_utils/expression_evaluator.hpp"
#include <rules/rule_config.pb.h>
#include <transaction/transaction.pb.h>

namespace fraud_detection {

class ThresholdRuleAnalyzer : public IRule {
public:
    explicit ThresholdRuleAnalyzer(const rules::RuleConfig& rule_config);

    bool IsFraudTransaction(const transaction::Transaction& transaction) const override;

private:
    using ExpressionValue = rule_utils::ExpressionValue;

    ExpressionValue EvaluateExpressionValue(
        const transaction::Transaction& transaction,
        const rules::Expression& expr) const;

    bool EvaluateComparison(
        const transaction::Transaction& transaction,
        const rules::ComparisonOperation& comp) const;

    const rules::RuleConfig rule_config_;
};

}