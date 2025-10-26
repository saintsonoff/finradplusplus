#pragma once

#include <userver/kafka/producer_component.hpp>
#include <rules/rule_result.pb.h>

namespace fraud_detection {

class KafkaResultProducer {
public:
    explicit KafkaResultProducer(userver::kafka::ProducerComponent& producer);

    void SendResult(const rules::RuleResult& result, const std::string& topic);

private:
    userver::kafka::ProducerComponent& producer_;
};

}  // namespace fraud_detection
