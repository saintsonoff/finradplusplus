#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include "rule_interface/IRule.hpp"
#include "transaction_history/transaction_history_service.hpp"
#include <rules/rule_config.pb.h>

namespace fraud_detection {

class RuleFactory {
public:
    static RulePtr CreateRuleByType(
        const rules::RuleConfig& config,
        std::shared_ptr<TransactionHistoryService> history_service = nullptr);

private:
    using RuleCreator = std::function<RulePtr(
        const rules::RuleConfig&,
        std::shared_ptr<TransactionHistoryService>)>;
    static const std::unordered_map<rules::RuleConfig_RuleType, RuleCreator>& GetCreators();
};

}
