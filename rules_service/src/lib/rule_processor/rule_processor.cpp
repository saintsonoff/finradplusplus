#include "rule_processor.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/redis/component.hpp>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

namespace fraud_detection {

RuleProcessor::RuleProcessor(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      consumer_(context.FindComponent<userver::kafka::ConsumerComponent>("kafka-consumer")),
      request_topic_(config["request_topic"].As<std::string>("Request")),
      result_service_endpoint_(config["result_service_endpoint"].As<std::string>("localhost:50051")),
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

    auto channel = grpc::CreateChannel(
        result_service_endpoint_,
        grpc::InsecureChannelCredentials()
    );
    result_service_stub_ = rules::ResultService::NewStub(channel);
    
    LOG_INFO() << "RuleProcessor initialized. Listening to topic: " << request_topic_;
    LOG_INFO() << "Result service endpoint: " << result_service_endpoint_;
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
    rules::RuleRequest request;
    if (!request.ParseFromString(message)) {
        LOG_ERROR() << "Failed to parse RuleRequest from message";
        
        rules::RuleReult error_result;
        error_result.set_status(rules::RuleReult::ERROR);
        error_result.set_profile_uuid("");
        error_result.set_profile_name("");
        error_result.set_config_uiid("");
        error_result.set_config_name("Failed to parse request");
        
        SendResultToService(error_result);
        return;
    }
    
    LOG_INFO() << "Processing rule: " << request.rule().uuid() 
               << " for transaction: " << request.transaction().transaction_id();
    
    rules::RuleReult result;
    result.set_profile_uuid(request.profile_uuid());
    result.set_profile_name(request.profile_name());
    result.set_config_uiid(request.rule().uuid());
    result.set_config_name(request.rule().name());
    
    try {
        if (history_service_) {
            history_service_->SaveTransaction(request.transaction());
            LOG_DEBUG() << "Saved transaction " << request.transaction().transaction_id() 
                       << " to Redis history";
        }
        
        auto rule = RuleFactory::CreateRuleByType(request.rule(), history_service_);
        bool is_fraud = rule->IsFraudTransaction(request.transaction());
        
        if (is_fraud) {
            result.set_status(rules::RuleReult::FRAUD);
            LOG_WARNING() << "FRAUD detected for transaction: " << request.transaction().transaction_id()
                         << " by rule: " << request.rule().uuid();
        } else {
            result.set_status(rules::RuleReult::NOT_FRAUD);
            LOG_INFO() << "Transaction " << request.transaction().transaction_id() 
                      << " is NOT FRAUD according to rule: " << request.rule().uuid();
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error evaluating rule " << request.rule().uuid() 
                   << ": " << e.what();
        result.set_status(rules::RuleReult::ERROR);
    }
    
    SendResultToService(result);
}

void RuleProcessor::SendResultToService(const rules::RuleReult& result) {
    grpc::ClientContext context;
    rules::SendResultResponse response;
    
    grpc::Status status = result_service_stub_->SendResult(&context, result, &response);
    
    if (status.ok()) {
        if (response.success()) {
            LOG_INFO() << "Successfully sent result to service for config: " 
                      << result.config_uiid() << ", status: " << result.status();
        } else {
            LOG_WARNING() << "Result sent but service returned failure: " 
                         << response.message();
        }
    } else {
        LOG_ERROR() << "Failed to send result to service: " 
                   << status.error_code() << ": " << status.error_message();
    }
}

}  // namespace fraud_detection
