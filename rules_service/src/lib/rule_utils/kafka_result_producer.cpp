#include "kafka_result_producer.hpp"

#include <google/protobuf/util/json_util.h>

#include <userver/logging/log.hpp>

namespace fraud_detection {

KafkaResultProducer::KafkaResultProducer(userver::kafka::ProducerComponent& producer)
    : producer_(producer) {
}

void KafkaResultProducer::SendResult(const rules::RuleResult& result, const std::string& topic) {
    std::string serialized_result;
    auto status = google::protobuf::util::MessageToJsonString(result, &serialized_result);
    if (!status.ok()) {
        LOG_ERROR() << "Failed to serialize RuleResult to JSON: " << status.message();
        return;
    }

    try {
        producer_.GetProducer().Send(topic, result.transaction_id(), serialized_result);
        LOG_INFO() << "Sent result to Kafka topic '" << topic << "' for transaction: " << result.transaction_id();
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to send result to Kafka: " << e.what();
    }
}

}  // namespace fraud_detection
