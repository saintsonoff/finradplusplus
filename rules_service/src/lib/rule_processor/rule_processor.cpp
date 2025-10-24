#include "rule_processor.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/redis/component.hpp>

namespace fraud_detection {

RuleProcessor::RuleProcessor(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      consumer_(context.FindComponent<userver::kafka::ConsumerComponent>("kafka-consumer")),
      producer_(context.FindComponent<userver::kafka::ProducerComponent>("kafka-producer")),
      request_topic_("Request"),
      response_topic_("Response"),
      consumer_scope_(consumer_.GetConsumer()) {
    try {
        auto& redis_component = context.FindComponent<userver::components::Redis>("redis");
        auto redis_client = redis_component.GetClient("transaction-history");
        history_service_ = std::make_shared<TransactionHistoryService>(redis_client);
        LOG_INFO() << "TransactionHistoryService initialized with Redis";
    } catch (const std::exception& e) {
        LOG_WARNING() << "Redis not available, TransactionHistoryService disabled: " << e.what();
        history_service_ = nullptr;
    }
    LOG_INFO() << "RuleProcessor initialized. Listening to topic: " << request_topic_;
    consumer_scope_.Start([this](userver::kafka::MessageBatchView messages) {
        LOG_INFO() << "Received batch of " << messages.size() << " messages";
        for (const auto& msg : messages) {
            try {
                std::string message_data(msg.GetPayload().begin(), msg.GetPayload().end());
                ProcessMessage(message_data);
            } catch (const std::exception& e) {
                LOG_ERROR() << "Error processing Kafka message: " << e.what();
            }
        }
    });
    LOG_INFO() << "RuleProcessor consumer started and ready to receive messages";
}

RuleProcessor::~RuleProcessor() {
    LOG_INFO() << "RuleProcessor shutting down";
}

void RuleProcessor::ProcessMessage(const std::string& message) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    requests::RuleRequest request;
    if (!request.ParseFromString(message)) {
        LOG_ERROR() << "Failed to parse RuleRequest from message";
        responses::RuleResponse error_response;
        error_response.set_status(responses::RuleResponse::ERROR);
        error_response.set_description("Failed to parse request");
        std::string serialized;
        error_response.SerializeToString(&serialized);
        producer_.GetProducer().Send(response_topic_, "", serialized);
        return;
    }
    LOG_INFO() << "Processing rule: " << request.rule().uuid() 
               << " for transaction: " << request.transaction().transaction_id();
    responses::RuleResponse response;
    response.set_rule_uuid(request.rule().uuid());
    try {
        if (history_service_) {
            history_service_->SaveTransaction(request.transaction());
            LOG_DEBUG() << "Saved transaction " << request.transaction().transaction_id() 
                       << " to Redis history";
        }
        auto rule = RuleFactory::CreateRuleByType(request.rule(), history_service_);
        bool is_fraud = rule->IsFraudTransaction(request.transaction());
        if (is_fraud) {
            response.set_status(responses::RuleResponse::FRAUD);
            response.set_description("Transaction flagged as fraudulent by rule: " + request.rule().name());
            LOG_WARNING() << "FRAUD detected for transaction: " << request.transaction().transaction_id()
                         << " by rule: " << request.rule().uuid();
        } else {
            response.set_status(responses::RuleResponse::NOT_FRAUD);
            response.set_description("Transaction passed fraud check");
            LOG_INFO() << "Transaction " << request.transaction().transaction_id() 
                      << " is NOT FRAUD according to rule: " << request.rule().uuid();
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error evaluating rule " << request.rule().uuid() 
                   << ": " << e.what();
        response.set_status(responses::RuleResponse::ERROR);
        response.set_description(std::string("Error evaluating rule: ") + e.what());
    }
    std::string serialized_response;
    if (response.SerializeToString(&serialized_response)) {
        producer_.GetProducer().Send(response_topic_, "", serialized_response);
        LOG_INFO() << "Sent response for rule: " << request.rule().uuid() 
                  << " with status: " << response.status();
    } else {
        LOG_ERROR() << "Failed to serialize response for rule: " << request.rule().uuid();
    }
}

}  // namespace fraud_detection
