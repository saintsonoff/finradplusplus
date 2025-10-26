#include "rule_processor.hpp"

#include <sstream>
#include <iomanip>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/yaml_config/schema.hpp>
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
        auto& pg_component = context.FindComponent<userver::components::Postgres>("postgres-db-1");
        auto pg_cluster = pg_component.GetCluster();
        history_service_ = std::make_shared<TransactionHistoryService>(pg_cluster);
        history_provider_ = std::make_shared<RedisHistoryProvider>(history_service_);
        LOG_INFO() << "TransactionHistoryService initialized with PostgreSQL";
    } catch (const std::exception& e) {
        LOG_WARNING() << "PostgreSQL not available, TransactionHistoryService disabled: " << e.what();
        history_service_ = nullptr;
        history_provider_ = nullptr;
    }

    ml_detector_ = std::make_shared<MLFraudDetector>();
    model_config_dir_ = config["ml_model_config_dir"].As<std::string>(
        "/workspaces/repozitorij-dlya-raboty-7408/rules_service/model_configs");
    

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
        
        rules::RuleResult error_result;
        error_result.set_status(rules::RuleResult::ERROR);
        error_result.set_profile_uuid("");
        error_result.set_profile_name("");
        error_result.set_config_uuid("");
        error_result.set_config_name("Failed to parse request");
        error_result.set_transaction_id("");
        error_result.set_description("Failed to parse RuleRequest from Kafka message");
        
        SendResultToService(error_result);
        return;
    }
    
    LOG_INFO() << "Processing rule: " << request.rule().uuid() 
               << " for transaction: " << request.transaction().transaction_id();
    
    rules::RuleResult result;
    result.set_profile_uuid(request.profile_uuid());
    result.set_profile_name(request.profile_name());
    result.set_config_uuid(request.rule().uuid());
    result.set_config_name(request.rule().name());
    result.set_transaction_id(request.transaction().transaction_id());
    
    try {
        if (history_service_) {
            history_service_->SaveTransaction(request.transaction());
            LOG_DEBUG() << "Saved transaction " << request.transaction().transaction_id() 
                       << " to PostgreSQL history";
        }
        
        if (request.rule().rule_type() == rules::RuleConfig::ML && ml_detector_ && history_provider_) {
            std::string uuid = request.rule().uuid();
            if (!ml_detector_->LoadModelByUuid(model_config_dir_, uuid)) {
                result.set_status(rules::RuleResult::ERROR);
                result.set_description("Model config not found for uuid: " + uuid);
                LOG_ERROR() << "Model config not found for uuid: " << uuid;
            } else {
                double fraud_probability = ml_detector_->PredictFraudProbability(request.transaction(), *history_provider_);
                double threshold = request.rule().ml_rule().lower_bound();
                bool is_fraud = fraud_probability >= threshold;
                std::ostringstream desc;
                desc << "ML Fraud Probability: " << std::fixed << std::setprecision(4) << fraud_probability 
                     << " (threshold: " << threshold << ")";
                result.set_description(desc.str());
                if (is_fraud) {
                    result.set_status(rules::RuleResult::FRAUD);
                    LOG_WARNING() << "FRAUD detected for transaction: " << request.transaction().transaction_id()
                                 << " by ML rule with probability: " << fraud_probability;
                } else {
                    result.set_status(rules::RuleResult::NOT_FRAUD);
                    LOG_INFO() << "Transaction " << request.transaction().transaction_id() 
                              << " is NOT FRAUD (probability: " << fraud_probability << ")";
                }
            }
        } else {
            auto rule = RuleFactory::CreateRuleByType(request.rule(), history_service_, ml_detector_);
            bool is_fraud = rule->IsFraudTransaction(request.transaction());
            
            std::string description;
            if (request.rule().rule_type() == rules::RuleConfig::THRESHOLD) {
                description = "Threshold rule applied, amount: " + std::to_string(request.transaction().amount());
            } else if (request.rule().rule_type() == rules::RuleConfig::PATTERN) {
                description = "Pattern rule applied";
            } else {
                description = "Rule type: " + std::to_string(static_cast<int>(request.rule().rule_type()));
            }
            result.set_description(description);
            
            if (is_fraud) {
                result.set_status(rules::RuleResult::FRAUD);
                LOG_WARNING() << "FRAUD detected for transaction: " << request.transaction().transaction_id()
                             << " by rule: " << request.rule().uuid();
            } else {
                result.set_status(rules::RuleResult::NOT_FRAUD);
                LOG_INFO() << "Transaction " << request.transaction().transaction_id() 
                          << " is NOT FRAUD according to rule: " << request.rule().uuid();
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Error evaluating rule " << request.rule().uuid() 
                   << ": " << e.what();
        result.set_status(rules::RuleResult::ERROR);
        result.set_description(std::string("Error: ") + e.what());
    }
    
    SendResultToService(result);
}

void RuleProcessor::SendResultToService(const rules::RuleResult& result) {
    grpc::ClientContext context;
    rules::SendResultResponse response;
    
    grpc::Status status = result_service_stub_->SendResult(&context, result, &response);
    
    if (status.ok()) {
        if (response.success()) {
            LOG_INFO() << "Successfully sent result to service for config: " 
                      << result.config_uuid() << ", status: " << result.status();
        } else {
            LOG_WARNING() << "Result sent but service returned failure: " 
                         << response.message();
        }
    } else {
        LOG_ERROR() << "Failed to send result to service: " 
                   << status.error_code() << ": " << status.error_message();
    }
}

userver::yaml_config::Schema RuleProcessor::GetStaticConfigSchema() {
    return userver::yaml_config::impl::SchemaFromString(R"(
type: object
description: Rule processor component
additionalProperties: false
properties:
    request_topic:
        type: string
        description: Kafka topic for incoming rule requests
        defaultDescription: Request
    result_service_endpoint:
        type: string
        description: gRPC endpoint for sending results
        defaultDescription: localhost:50051
    ml_model_config_dir:
        type: string
        description: Directory containing ML model files
        defaultDescription: version_to_cpp_enjoyer/
)");
}

}  // namespace fraud_detection
