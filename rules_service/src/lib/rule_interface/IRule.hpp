#pragma once

#include <memory>
#include <transaction/transaction.pb.h>

namespace fraud_detection {

class IRule {
public:
    virtual ~IRule() = default;

    virtual bool IsFraudTransaction(
        const transaction::Transaction& transaction) const = 0;
};

using RulePtr = std::unique_ptr<IRule>;

}
