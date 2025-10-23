#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "rule_interface/IRule.hpp"
#include "threshold_rule/threshold_rule.hpp"
#include "pattern_rule/pattern_rule.hpp"
#include "ml_rule/ml_rule.hpp"
#include "composite_rule/composite_rule.hpp"
#include "rule_config.pb.h"

class RuleFactory {
public:
    static RulePtr CreateRuleByType(const rules::RuleConfig& config) {
        const auto& creators = GetCreators();
        auto it = creators.find(config.rule_type());
        
        if (it == creators.end()) {
            throw std::runtime_error(
                "Unknown RuleType: " + std::to_string(config.rule_type()));
        }
        
        return it->second(config);
    }
private:
    using RuleCreator = std::function<RulePtr(const rules::RuleConfig&)>;
    
    static const std::unordered_map<rules::RuleConfig_RuleType, RuleCreator>& GetCreators() {
        static const std::unordered_map<rules::RuleConfig_RuleType, RuleCreator> creators = {
            {rules::RuleConfig_RuleType_THRESHOLD, [](const rules::RuleConfig& config) -> RulePtr {
                if (!config.has_threshold_rule()) {
                    throw std::invalid_argument("RuleType is THRESHOLD but threshold_rule not set");
                }
                return std::make_unique<ThresholdRuleAnalyzer>(config);
            }},
            {rules::RuleConfig_RuleType_PATTERN, [](const rules::RuleConfig& config) -> RulePtr {
                if (!config.has_pattern_rule()) {
                    throw std::invalid_argument("RuleType is PATTERN but pattern_rule not set");
                }
                return std::make_unique<PatternRuleAnalyzer>(config);
            }},
            {rules::RuleConfig_RuleType_ML, [](const rules::RuleConfig& config) -> RulePtr {
                if (!config.has_ml_rule()) {
                    throw std::invalid_argument("RuleType is ML but ml_rule not set");
                }
                return std::make_unique<MlRuleAnalyzer>(config);
            }},
            {rules::RuleConfig_RuleType_COMPOSITE, [](const rules::RuleConfig& config) -> RulePtr {
                if (!config.has_composite_rule()) {
                    throw std::invalid_argument("RuleType is COMPOSITE but composite_rule not set");
                }
                return std::make_unique<CompositeRuleAnalyzer>(config);
            }}
        };
        return creators;
    }
};