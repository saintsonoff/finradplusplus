#include "rule_result_consumer.hpp"


// userver
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

#include <userver/kafka/consumer_scope.hpp>
#include <userver/kafka/consumer_component.hpp>

#include <userver/logging/log.hpp>


// models
#include <rules/rule_result.pb.h>

namespace reporter_service {


void RuleResultConsumer::operator()(userver::kafka::MessageBatchView messages) const {
    for (const auto& message : messages) {
        if (!message.GetTimestamp().has_value()) {
            continue;
        }

        rules::RuleResult rule_result;
        
        std::string serialized_data(message.GetPayload().begin(), message.GetPayload().end());
        
        if (!rule_result.ParseFromString(serialized_data)) {
            LOG_ERROR() 
                << fmt::format("RulesResultConsumer: failed to parse RuleResult from Kafka message, key: {}, topic: {}, partition: {}",
                    message.GetKey(), message.GetTopic(), message.GetPartition());
            continue;
        }
        
        LOG_INFO() << "Successfully parsed RuleResult: " << rule_result.DebugString();
    }
}


RuleResultConsumerComponent::RuleResultConsumerComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    )
  : userver::components::ComponentBase{config, context},
  _consumer{context.FindComponent<userver::kafka::ConsumerComponent>().GetConsumer()} {
        _consumer.Start([this](userver::kafka::MessageBatchView messages) {
            kRuleResultConsumer(messages);
            _consumer.AsyncCommit();
        });
    }


} // namespace reporter_service