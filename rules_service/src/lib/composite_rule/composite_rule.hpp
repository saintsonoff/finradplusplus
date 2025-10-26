#pragma once

#include "rule_interface/IRule.hpp"
#include "rule_utils/expression_evaluator.hpp"
#include <rules/rule_config.pb.h>
#include <transaction/transaction.pb.h>
#include <stdexcept>

namespace fraud_detection {

class CompositeRuleAnalyzer : public IRule {
public:
    explicit CompositeRuleAnalyzer(const rules::RuleConfig& rule_config)
        : rule_config_(rule_config) {}

    bool IsFraudTransaction(const transaction::Transaction& transaction) const override {
        if (!rule_config_.has_composite_rule()) {
            return false;
        }
        return EvaluateExpression(transaction, rule_config_.composite_rule().expression());
    }

private:
    using ExpressionValue = rule_utils::ExpressionValue;

    ExpressionValue EvaluateExpressionValue(
        const transaction::Transaction& transaction,
        const rules::Expression& expr) const {
        switch (expr.expr_case()) {
            case rules::Expression::kField:
                return rule_utils::FieldExtractor::GetFieldValue(
                    transaction, expr.field().field());
            case rules::Expression::kLiteral:
                return rule_utils::LiteralExtractor::GetLiteralValue(expr.literal());
            default:
                throw std::runtime_error("Cannot evaluate expression to value");
        }
    }

    bool EvaluateExpression(
        const transaction::Transaction& transaction,
        const rules::Expression& expr) const {
        switch (expr.expr_case()) {
            case rules::Expression::kComparison:
                return EvaluateComparison(transaction, expr.comparison());
            case rules::Expression::kLogical:
                return EvaluateLogical(transaction, expr.logical());
            case rules::Expression::kLiteral:
                if (expr.literal().value_case() == rules::LiteralValue::kBoolValue) {
                    return expr.literal().bool_value();
                }
                throw std::runtime_error("Expression is not boolean");
            default:
                throw std::runtime_error("Expression is not boolean");
        }
    }

    bool EvaluateComparison(
        const transaction::Transaction& transaction,
        const rules::ComparisonOperation& comp) const {
        auto left = EvaluateExpressionValue(transaction, comp.left());
        auto right = EvaluateExpressionValue(transaction, comp.right());
        return rule_utils::ComparisonEvaluator::Evaluate(left, right, comp.operator_());
    }

    bool EvaluateLogical(
        const transaction::Transaction& transaction,
        const rules::LogicalOperation& logical) const {
        switch (logical.operator_()) {
            case rules::LogicalOperation::AND:
                return EvaluateAnd(transaction, logical);
            case rules::LogicalOperation::OR:
                return EvaluateOr(transaction, logical);
            case rules::LogicalOperation::NOT:
                return EvaluateNot(transaction, logical);
            default:
                throw std::runtime_error("Unknown logical operator");
        }
    }

    bool EvaluateAnd(
        const transaction::Transaction& transaction,
        const rules::LogicalOperation& logical) const {
        for (const auto& operand : logical.operands()) {
            if (!EvaluateExpression(transaction, operand)) {
                return false;
            }
        }
        return true;
    }

    bool EvaluateOr(
        const transaction::Transaction& transaction,
        const rules::LogicalOperation& logical) const {
        for (const auto& operand : logical.operands()) {
            if (EvaluateExpression(transaction, operand)) {
                return true;
            }
        }
        return false;
    }

    bool EvaluateNot(
        const transaction::Transaction& transaction,
        const rules::LogicalOperation& logical) const {
        if (logical.operands_size() != 1) {
            throw std::runtime_error("NOT operator requires exactly one operand");
        }
        return !EvaluateExpression(transaction, logical.operands(0));
    }

    const rules::RuleConfig rule_config_;
};

}
