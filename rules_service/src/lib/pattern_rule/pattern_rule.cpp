#include "pattern_rule.hpp"
#include <stdexcept>
#include <unordered_set>
#include <limits>
#include <cmath>
#include <algorithm>

namespace fraud_detection {

PatternRuleAnalyzer::PatternRuleAnalyzer(
    const rules::RuleConfig& rule_config,
    std::shared_ptr<TransactionHistoryService> history_service)
    : rule_config_(rule_config)
    , history_service_(std::move(history_service)) {}

bool PatternRuleAnalyzer::IsFraudTransaction(const transaction::Transaction& transaction) const {
    if (!rule_config_.has_pattern_rule()) {
        return false;
    }
    
    return EvaluateExpression(transaction, rule_config_.pattern_rule().expression());
}


rule_utils::ExpressionValue PatternRuleAnalyzer::EvaluateExpressionValue(
    const transaction::Transaction& transaction,
    const rules::Expression& expr) const {
    
    switch (expr.expr_case()) {
        case rules::Expression::kField:
            return rule_utils::FieldExtractor::GetFieldValue(
                transaction, expr.field().field());
        
        case rules::Expression::kLiteral:
            return rule_utils::LiteralExtractor::GetLiteralValue(expr.literal());
        
        case rules::Expression::kAggregate:
            return EvaluateAggregate(transaction, expr.aggregate());
        
        default:
            throw std::runtime_error("Cannot evaluate expression to value");
    }
}

bool PatternRuleAnalyzer::EvaluateExpression(
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

bool PatternRuleAnalyzer::EvaluateComparison(
    const transaction::Transaction& transaction,
    const rules::ComparisonOperation& comp) const {
    
    auto left = EvaluateExpressionValue(transaction, comp.left());
    auto right = EvaluateExpressionValue(transaction, comp.right());
    
    return rule_utils::ComparisonEvaluator::Evaluate(left, right, comp.operator_());
}

bool PatternRuleAnalyzer::EvaluateLogical(
    const transaction::Transaction& transaction,
    const rules::LogicalOperation& logical) const {
    
    switch (logical.operator_()) {
        case rules::LogicalOperation::AND:
            for (const auto& operand : logical.operands()) {
                if (!EvaluateExpression(transaction, operand)) {
                    return false;
                }
            }
            return true;
        
        case rules::LogicalOperation::OR:
            for (const auto& operand : logical.operands()) {
                if (EvaluateExpression(transaction, operand)) {
                    return true;
                }
            }
            return false;
        
        case rules::LogicalOperation::NOT:
            if (logical.operands_size() != 1) {
                throw std::runtime_error("NOT requires exactly one operand");
            }
            return !EvaluateExpression(transaction, logical.operands(0));
        
        default:
            throw std::runtime_error("Unknown logical operator");
    }
}


rule_utils::ExpressionValue PatternRuleAnalyzer::EvaluateAggregate(
    const transaction::Transaction& transaction,
    const rules::AggregateFunction& agg) const {
    if (!history_service_) throw std::runtime_error("No history service for SQL aggregate");

    std::string sender_account = transaction.sender_account();
    std::string sql;
    // pass single param (sender_account) to ExecuteAggregateQuery to avoid text[] being sent
    // Do NOT add SQL quotes here — driver will bind the parameter correctly.
    std::string param = sender_account;
    std::string field_name;
    if (agg.operand().expr_case() == rules::Expression::kField) {
        switch (agg.operand().field().field()) {
            case rules::FieldReference::AMOUNT: field_name = "amount"; break;
            case rules::FieldReference::MERCHANT_CATEGORY: field_name = "merchant_category"; break;
            case rules::FieldReference::LOCATION: field_name = "location"; break;
            case rules::FieldReference::DEVICE_USED: field_name = "device_used"; break;
            case rules::FieldReference::PAYMENT_CHANNEL: field_name = "payment_channel"; break;
            case rules::FieldReference::TRANSACTION_TYPE: field_name = "transaction_type"; break;
            case rules::FieldReference::RECEIVER_ACCOUNT: field_name = "receiver_account"; break;
            case rules::FieldReference::SENDER_ACCOUNT: field_name = "sender_account"; break;
            default: throw std::runtime_error("Unsupported field for SQL aggregate");
        }
    }

    switch (agg.function()) {
        case rules::AggregateFunction::COUNT:
            sql = "SELECT COUNT(*) FROM transactions WHERE sender_account = $1";
            break;
        case rules::AggregateFunction::SUM:
            sql = "SELECT SUM(" + field_name + ") FROM transactions WHERE sender_account = $1";
            break;
        case rules::AggregateFunction::AVG:
            sql = "SELECT AVG(" + field_name + ") FROM transactions WHERE sender_account = $1";
            break;
        case rules::AggregateFunction::MIN:
            sql = "SELECT MIN(" + field_name + ") FROM transactions WHERE sender_account = $1";
            break;
        case rules::AggregateFunction::MAX:
            sql = "SELECT MAX(" + field_name + ") FROM transactions WHERE sender_account = $1";
            break;
        case rules::AggregateFunction::COUNT_DISTINCT:
            sql = "SELECT COUNT(DISTINCT " + field_name + ") FROM transactions WHERE sender_account = $1";
            break;
        default:
            throw std::runtime_error("Unknown aggregate function");
    }
    float result = history_service_->ExecuteAggregateQuery(sql, param);
    if (agg.function() == rules::AggregateFunction::COUNT || agg.function() == rules::AggregateFunction::COUNT_DISTINCT) {
        return static_cast<int32_t>(result);
    } else {
        return result;
    }
}


} // namespace fraud_detection
