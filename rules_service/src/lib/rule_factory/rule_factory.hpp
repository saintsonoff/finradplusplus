#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "rule_interface/IRule.hpp"
#include "threshold_rule/threshold_rule.hpp"
#include "pattern_rule/pattern_rule.hpp"
#include "ml_rule/ml_rule.hpp"
#include "composite_rule/composite_rule.hpp"
#include "rule_config.pb.h"

class RuleFactory {
public:
    static RulePtr CreateRuleByType(const rules::RuleConfig& config) {
        switch (config.rule_type()) {
            case rules::RuleConfig_RuleType_THRESHOLD:
                if (!config.has_threshold_rule()) {
                    throw std::invalid_argument(
                        "RuleType is THRESHOLD but threshold_rule not set");
                }
                return std::make_unique<ThresholdRuleAnalyzer>(config);

            case rules::RuleConfig_RuleType_PATTERN:
                if (!config.has_pattern_rule()) {
                    throw std::invalid_argument(
                        "RuleType is PATTERN but pattern_rule not set");
                }
                return std::make_unique<PatternRuleAnalyzer>(config);

            case rules::RuleConfig_RuleType_ML:
                if (!config.has_ml_rule()) {
                    throw std::invalid_argument(
                        "RuleType is ML but ml_rule not set");
                }
                return std::make_unique<MlRuleAnalyzer>(config);

            case rules::RuleConfig_RuleType_COMPOSITE:
                if (!config.has_composite_rule()) {
                    throw std::invalid_argument(
                        "RuleType is COMPOSITE but composite_rule not set");
                }
                return std::make_unique<CompositeRuleAnalyzer>(config);

            default:
                throw std::runtime_error(
                    "Unknown RuleType: " +
                    std::to_string(config.rule_type()));
        }
    }
};