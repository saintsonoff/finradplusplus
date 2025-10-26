#include "rule_factory.hpp"
#include <stdexcept>
#include <string>
#include "threshold_rule/threshold_rule.hpp"
#include "pattern_rule/pattern_rule.hpp"
#include "ml_rule/ml_rule.hpp"
#include "composite_rule/composite_rule.hpp"

namespace fraud_detection {

RulePtr RuleFactory::CreateRuleByType(
    const rules::RuleConfig& config,
    std::shared_ptr<TransactionHistoryService> history_service,
    std::shared_ptr<MLFraudDetector> ml_detector) {
    const auto& creators = GetCreators();
    auto it = creators.find(config.rule_type());
    
    if (it == creators.end()) {
        throw std::runtime_error(
            "Unknown RuleType: " + std::to_string(config.rule_type()));
    }
    
    return it->second(config, history_service, ml_detector);
}

const std::unordered_map<rules::RuleConfig_RuleType, RuleFactory::RuleCreator>& 
RuleFactory::GetCreators() {
    static const std::unordered_map<rules::RuleConfig_RuleType, RuleCreator> creators = {
        {rules::RuleConfig_RuleType_THRESHOLD, [](const rules::RuleConfig& config, auto, auto) -> RulePtr {
            if (!config.has_threshold_rule()) {
                throw std::invalid_argument("RuleType is THRESHOLD but threshold_rule not set");
            }
            return std::make_unique<ThresholdRuleAnalyzer>(config);
        }},
        {rules::RuleConfig_RuleType_PATTERN, [](const rules::RuleConfig& config, 
                std::shared_ptr<TransactionHistoryService> history_service, auto) -> RulePtr {
            if (!config.has_pattern_rule()) {
                throw std::invalid_argument("RuleType is PATTERN but pattern_rule not set");
            }
            if (!history_service) {
                throw std::invalid_argument("PATTERN rule requires TransactionHistoryService");
            }
            return std::make_unique<PatternRuleAnalyzer>(config, history_service);
        }},
        {rules::RuleConfig_RuleType_ML, [](const rules::RuleConfig& config, 
                std::shared_ptr<TransactionHistoryService> history_service,
                std::shared_ptr<MLFraudDetector> ml_detector) -> RulePtr {
            if (!config.has_ml_rule()) {
                throw std::invalid_argument("RuleType is ML but ml_rule not set");
            }
            if (!ml_detector) {
                throw std::invalid_argument("ML rule requires MLFraudDetector");
            }
            if (!history_service) {
                throw std::invalid_argument("ML rule requires TransactionHistoryService for feature extraction");
            }
            // Создать history provider для ML детектора
            auto history_provider = std::make_shared<RedisHistoryProvider>(history_service);
            return std::make_unique<MlRuleAnalyzer>(config, ml_detector, history_provider);
        }},
        {rules::RuleConfig_RuleType_COMPOSITE, [](const rules::RuleConfig& config, auto, auto) -> RulePtr {
            if (!config.has_composite_rule()) {
                throw std::invalid_argument("RuleType is COMPOSITE but composite_rule not set");
            }
            return std::make_unique<CompositeRuleAnalyzer>(config);
        }}
    };
    return creators;
}

}
