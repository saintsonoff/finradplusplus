#pragma once

#include "rule_interface/IRule.hpp"
#include "rule_utils/expression_evaluator.hpp"
#include "transaction_history/transaction_history_service.hpp"
#include <rules/rule_config.pb.h>
#include <transaction/transaction.pb.h>
#include <vector>
#include <memory>

namespace fraud_detection {

class PatternRuleAnalyzer : public IRule {
public:
    explicit PatternRuleAnalyzer(
        const rules::RuleConfig& rule_config,
        std::shared_ptr<TransactionHistoryService> history_service);

    bool IsFraudTransaction(const transaction::Transaction& transaction) const override;

private:
    using ExpressionValue = rule_utils::ExpressionValue;

    std::vector<transaction::Transaction> GetRelevantTransactions(
        const transaction::Transaction& current_transaction) const;

    ExpressionValue EvaluateExpressionValue(
        const transaction::Transaction& transaction,
        const rules::Expression& expr) const;

    bool EvaluateExpression(
        const transaction::Transaction& transaction,
        const rules::Expression& expr) const;

    bool EvaluateComparison(
        const transaction::Transaction& transaction,
        const rules::ComparisonOperation& comp) const;

    bool EvaluateLogical(
        const transaction::Transaction& transaction,
        const rules::LogicalOperation& logical) const;

    ExpressionValue EvaluateAggregate(
        const transaction::Transaction& transaction,
        const rules::AggregateFunction& agg) const;

    float ComputeSum(
        const std::vector<transaction::Transaction>& txs,
        rules::FieldReference::FieldType field) const;

    float ComputeAvg(
        const std::vector<transaction::Transaction>& txs,
        rules::FieldReference::FieldType field) const;

    float ComputeMin(
        const std::vector<transaction::Transaction>& txs,
        rules::FieldReference::FieldType field) const;

    float ComputeMax(
        const std::vector<transaction::Transaction>& txs,
        rules::FieldReference::FieldType field) const;

    int32_t ComputeCountDistinct(
        const std::vector<transaction::Transaction>& txs,
        rules::FieldReference::FieldType field) const;

    const rules::RuleConfig rule_config_;
    std::shared_ptr<TransactionHistoryService> history_service_;
};

}
