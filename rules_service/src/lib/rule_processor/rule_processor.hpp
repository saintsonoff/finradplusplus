#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>

#include <rules/rule_request.pb.h>
#include <rules/rule_result.pb.h>
#include <rules/result_service.grpc.pb.h>
#include "rule_factory/rule_factory.hpp"
#include "transaction_history/transaction_history_service.hpp"
#include "ml_model/ml_fraud_detector.hpp"
#include "ml_model/redis_history_provider.hpp"

namespace fraud_detection {

class RuleProcessor final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "rule-processor";

    RuleProcessor(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    ~RuleProcessor() override;

    RuleProcessor(const RuleProcessor&) = delete;
    RuleProcessor& operator=(const RuleProcessor&) = delete;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    void ProcessMessage(const std::string& message);
    void SendResultToService(const rules::RuleResult& result);
    
    userver::kafka::ConsumerComponent& consumer_;
    
    std::string request_topic_;
    std::string result_service_endpoint_;
    
    userver::kafka::ConsumerScope consumer_scope_;
    
    std::shared_ptr<TransactionHistoryService> history_service_;
    std::shared_ptr<RedisHistoryProvider> history_provider_;
    std::unique_ptr<rules::ResultService::Stub> result_service_stub_;
    std::shared_ptr<MLFraudDetector> ml_detector_;
    std::string model_config_dir_;
};

}  // namespace fraud_detection
