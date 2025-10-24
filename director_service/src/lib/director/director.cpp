#include "director.hpp"

// stdcpp
#include <string>

//userver
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/kafka/producer.hpp>
#include <userver/kafka/producer_component.hpp>

#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>


// self
#include <rule_request_producer.hpp>
#include <rules/profile.pb.h>
#include <transaction/transaction.pb.h>


bool operator==(const profile::Profile& lhs, const profile::Profile& rhs) {
    return lhs.uuid() == rhs.uuid();
}

namespace director_service {


Director::Director(const userver::kafka::Producer& producer, std::string topic)
    : _topic(std::move(topic)), _producer(producer), _profiles{} {  };

void Director::ProcessTransaction(transaction::Transaction transaction) {
    for (auto profile : _profiles) {
        kRuleRequestProducer(_topic, _producer, profile, transaction);
    }
}


DirectorComponent::DirectorComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) 
    : userver::components::ComponentBase{config, context},
    _director{
        context.FindComponent<userver::kafka::ProducerComponent>().GetProducer(),
        config[kName].As<std::string>("topic")
    } {
    LOG_INFO() << "Director component start successfully with topic: " << config[kName].As<std::string>("topic");
}

userver::yaml_config::Schema DirectorComponent::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
type: object
description: Director component for business logic processing
additionalProperties: false
properties:
    topic:
        type: string
        description: Kafka topic name for rule requests
)");
// task-processor:
//     type: string
//     description: Task processor for async operations
}


} // namespace director_service