#pragma once

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/producer_component.hpp>
#include <userver/storages/redis/client.hpp>

#include <requests/rule_request.pb.h>
#include <responses/rule_response.pb.h>
#include "rule_factory/rule_factory.hpp"
#include "transaction_history/transaction_history_service.hpp"

namespace fraud_detection {

class RuleProcessor final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "rule-processor";

    RuleProcessor(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    ~RuleProcessor() override;

private:
    void ProcessMessage(const std::string& message);
    
    userver::kafka::ConsumerComponent& consumer_;
    userver::kafka::ProducerComponent& producer_;
    
    std::string request_topic_;
    std::string response_topic_;
    
    // Keep consumer scope alive to receive messages
    userver::kafka::ConsumerScope consumer_scope_;
    
    // Transaction history service for Redis
    std::shared_ptr<TransactionHistoryService> history_service_;
};

}  // namespace fraud_detection
