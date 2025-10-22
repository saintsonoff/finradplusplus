#pragma once
#include "transaction.pb.h"
#include "rules/threshold_rule.pb.h"
namespace threshold {
class ThresholdValidator {
public:
    explicit ThresholdValidator(const ::rules::ThresholdRule&) {}
    bool Validate(const transaction::Transaction&) const { return true; }
};
}
