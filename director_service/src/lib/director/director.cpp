#include "director.hpp"

//userver
#include <userver/components/component_base.hpp>
#include <userver/components/component_context.hpp>

#include <userver/kafka/producer.hpp>
#include <userver/kafka/producer_component.hpp>

#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

namespace director_service {


Director::Director(const userver::kafka::Producer& producer)
    : _producer(producer) {  };


DirectorComponent::DirectorComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) 
    : userver::components::LoggableComponentBase{config, context},
    _director{context.FindComponent<userver::kafka::ProducerComponent>().GetProducer()} {
    LOG_INFO() << "Director component start successfully";
}

userver::yaml_config::Schema DirectorComponent::GetStaticConfigSchema() {
    return userver::yaml_config::Schema{R"(
type: object
description: Director component for on-demand task processing
additionalProperties: false
properties:
    task-processor:
        type: string
        description: Task processor for async operations
)"};
};


} // namespace director_service