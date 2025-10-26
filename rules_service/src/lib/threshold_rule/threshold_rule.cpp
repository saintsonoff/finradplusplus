#include "threshold_rule.hpp"
#include <stdexcept>

namespace fraud_detection {

ThresholdRuleAnalyzer::ThresholdRuleAnalyzer(const rules::RuleConfig& rule_config)
    : rule_config_(rule_config) {}

bool ThresholdRuleAnalyzer::IsFraudTransaction(const transaction::Transaction& transaction) const {
    if (!rule_config_.has_threshold_rule()) {
        return false;
    }
    
    const auto& expr = rule_config_.threshold_rule().expression();
    
    if (expr.expr_case() != rules::Expression::kComparison) {
        throw std::runtime_error("ThresholdRule supports only comparison operations");
    }
    
    return EvaluateComparison(transaction, expr.comparison());
}

rule_utils::ExpressionValue ThresholdRuleAnalyzer::EvaluateExpressionValue(
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

bool ThresholdRuleAnalyzer::EvaluateComparison(
    const transaction::Transaction& transaction,
    const rules::ComparisonOperation& comp) const {
    
    auto left = EvaluateExpressionValue(transaction, comp.left());
    auto right = EvaluateExpressionValue(transaction, comp.right());
    
    return rule_utils::ComparisonEvaluator::Evaluate(left, right, comp.operator_());
}

}