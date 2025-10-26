#pragma once


// stdcpp
#include <utility>

// userver
#include <userver/kafka/producer.hpp>
#include <userver/kafka/exceptions.hpp>

// self
#include <rules/profile.pb.h>
#include <transaction/transaction.pb.h>

namespace director_service {


class RuleRequestProducer {
public:
    enum class SendStatus {
        kSuccess,
        kErrorSerializationNonRetryable,
        kErrorRetryable,
        kErrorNonRetryable,
    };

public:
    constexpr RuleRequestProducer() = default;

public:
    std::pair<size_t, SendStatus> operator()(
        const std::string& topic,
        const userver::kafka::Producer& producer,
        profile::Profile& profile,
        transaction::Transaction& transaction
    ) const;
};


inline constexpr RuleRequestProducer kRuleRequestProducer = {};

} // namespace director_service