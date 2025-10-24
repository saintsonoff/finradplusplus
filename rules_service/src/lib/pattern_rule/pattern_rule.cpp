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

std::vector<transaction::Transaction> PatternRuleAnalyzer::GetRelevantTransactions(
    const transaction::Transaction& current_transaction) const {
    
    if (!history_service_) {
        return {};
    }
    
    const std::string& sender_account = current_transaction.sender_account();
    int32_t max_delta = rule_config_.pattern_rule().max_delta_time();
    
    if (max_delta > 0) {
        int minutes = max_delta / 60;
        return history_service_->GetRecentTransactions(sender_account, minutes, 100);
    }
    
    return history_service_->GetAccountHistory(sender_account, 100);
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

    auto relevant_txs = GetRelevantTransactions(transaction);

    switch (agg.function()) {
        case rules::AggregateFunction::COUNT:
            return static_cast<int32_t>(relevant_txs.size());

        case rules::AggregateFunction::SUM:
            if (agg.operand().expr_case() != rules::Expression::kField) {
                throw std::runtime_error("SUM aggregate requires a field operand");
            }
            return ComputeSum(relevant_txs, agg.operand().field().field());

        case rules::AggregateFunction::AVG:
            if (agg.operand().expr_case() != rules::Expression::kField) {
                throw std::runtime_error("AVG aggregate requires a field operand");
            }
            return ComputeAvg(relevant_txs, agg.operand().field().field());

        case rules::AggregateFunction::MIN:
            if (agg.operand().expr_case() != rules::Expression::kField) {
                throw std::runtime_error("MIN aggregate requires a field operand");
            }
            return ComputeMin(relevant_txs, agg.operand().field().field());

        case rules::AggregateFunction::MAX:
            if (agg.operand().expr_case() != rules::Expression::kField) {
                throw std::runtime_error("MAX aggregate requires a field operand");
            }
            return ComputeMax(relevant_txs, agg.operand().field().field());

        case rules::AggregateFunction::COUNT_DISTINCT:
            if (agg.operand().expr_case() != rules::Expression::kField) {
                throw std::runtime_error("COUNT_DISTINCT requires a field operand");
            }
            return ComputeCountDistinct(relevant_txs, agg.operand().field().field());

        default:
            throw std::runtime_error("Unknown aggregate function");
    }
}

float PatternRuleAnalyzer::ComputeSum(
    const std::vector<transaction::Transaction>& txs,
    rules::FieldReference::FieldType field) const {
    
    float sum = 0.0f;
    for (const auto& tx : txs) {
        auto value = rule_utils::FieldExtractor::GetFieldValue(tx, field);
        
        if (std::holds_alternative<float>(value)) {
            sum += std::get<float>(value);
        } else if (std::holds_alternative<int32_t>(value)) {
            sum += std::get<int32_t>(value);
        }
    }
    return sum;
}

float PatternRuleAnalyzer::ComputeAvg(
    const std::vector<transaction::Transaction>& txs,
    rules::FieldReference::FieldType field) const {
    
    if (txs.empty()) return 0.0f;
    return ComputeSum(txs, field) / txs.size();
}

float PatternRuleAnalyzer::ComputeMin(
    const std::vector<transaction::Transaction>& txs,
    rules::FieldReference::FieldType field) const {
    
    if (txs.empty()) {
        throw std::runtime_error("MIN on empty set");
    }

    float min_val = std::numeric_limits<float>::max();
    for (const auto& tx : txs) {
        auto value = rule_utils::FieldExtractor::GetFieldValue(tx, field);
        
        if (std::holds_alternative<float>(value)) {
            min_val = std::min(min_val, std::get<float>(value));
        } else if (std::holds_alternative<int32_t>(value)) {
            min_val = std::min(min_val, static_cast<float>(std::get<int32_t>(value)));
        }
    }
    return min_val;
}

float PatternRuleAnalyzer::ComputeMax(
    const std::vector<transaction::Transaction>& txs,
    rules::FieldReference::FieldType field) const {
    
    if (txs.empty()) {
        throw std::runtime_error("MAX on empty set");
    }

    float max_val = std::numeric_limits<float>::lowest();
    for (const auto& tx : txs) {
        auto value = rule_utils::FieldExtractor::GetFieldValue(tx, field);
        
        if (std::holds_alternative<float>(value)) {
            max_val = std::max(max_val, std::get<float>(value));
        } else if (std::holds_alternative<int32_t>(value)) {
            max_val = std::max(max_val, static_cast<float>(std::get<int32_t>(value)));
        }
    }
    return max_val;
}

int32_t PatternRuleAnalyzer::ComputeCountDistinct(
    const std::vector<transaction::Transaction>& txs,
    rules::FieldReference::FieldType field) const {
    
    std::unordered_set<std::string> distinct_values;
    
    for (const auto& tx : txs) {
        auto value = rule_utils::FieldExtractor::GetFieldValue(tx, field);
        
        std::string str_value;
        if (std::holds_alternative<std::string>(value)) {
            str_value = std::get<std::string>(value);
        } else if (std::holds_alternative<float>(value)) {
            str_value = std::to_string(std::get<float>(value));
        } else if (std::holds_alternative<int32_t>(value)) {
            str_value = std::to_string(std::get<int32_t>(value));
        } else if (std::holds_alternative<bool>(value)) {
            str_value = std::get<bool>(value) ? "true" : "false";
        }
        
        distinct_values.insert(str_value);
    }
    
    return static_cast<int32_t>(distinct_values.size());
}

}
