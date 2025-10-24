#pragma once


// stdcpp
#include <utility>
#include <string>

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
    RuleRequestProducer(std::string topic_name);

public:
    std::pair<size_t, SendStatus> operator()(
        const userver::kafka::Producer& producer,
        profile::Profile profile,
        transaction::Transaction transaction
    );

private:
    std::string _topic;
};


} // namespace director_service