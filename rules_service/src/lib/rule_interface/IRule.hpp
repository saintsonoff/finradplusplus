#pragma once

#include <memory>
#include "rule_config.pb.h"
#include "transaction.pb.h"

class IRule {
public:
    virtual ~IRule() = default;

    virtual bool IsFraudTransaction(
        const transaction::Transaction& transaction) const = 0;
};

using RulePtr = std::unique_ptr<IRule>;
