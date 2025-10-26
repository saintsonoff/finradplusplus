#include "director.hpp"

// stdcpp
#include <string>
#include <mutex>
#include <utility>

//userver
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/kafka/producer.hpp>
#include <userver/kafka/producer_component.hpp>

#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/task.hpp>

// another
#include <fmt/format.h>

// self
#include <rule_request_producer.hpp>
#include <rules/profile.pb.h>
#include <transaction/transaction.pb.h>


namespace director_service {

Director::Director(
    std::string topic,
    const userver::kafka::Producer& producer)
  : _topic(std::move(topic)), _producer(producer)
    {
}

void Director::UpdateProfiles(Director::ProfileContainer profiles) {
    LOG_INFO() << "Old profiles count: " << _profiles.size();
    _profiles = std::move(profiles);
    LOG_INFO() << "New profiles count: " << _profiles.size();
}


void Director::ProcessTransaction(
  transaction::Transaction&& transaction,
  userver::engine::TaskProcessor& task_processor
) const {
    userver::engine::DetachUnscopedUnsafe(userver::engine::AsyncNoSpan(task_processor,
    [topic = _topic, transaction = transaction, profiles = _profiles, &producer = _producer]() mutable -> void {
        LOG_INFO()
             << fmt::format("Director start transaction processing process: transaction_id: {}, profiles_count: {}",
                    transaction.transaction_id(), profiles.size());

        for (auto profile : profiles) {
            auto [produce_count, produce_result] = kRuleRequestProducer(topic, producer, profile, transaction);
            if (produce_result != RuleRequestProducer::SendStatus::kSuccess) {
                LOG_ERROR()
                    << fmt::format(
                        "Transaction producing error: {} from {}, produce_status: {}", produce_count, profile.rules().size(),
                            produce_result == RuleRequestProducer::SendStatus::kErrorRetryable ? "retryable" : "nonretryable"
                    );
            }
            LOG_INFO()
                    << fmt::format("Produce trancation: transaction_id: {}, profile_id: {}, config_count: {}",
                        transaction.transaction_id(), profile.uuid(), profile.rules().size());
        }
        LOG_INFO() << "Director end transaction processing process";
    }));
}


DirectorComponent::DirectorComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) 
    : userver::components::ComponentBase{config, context},
      _task_processor{context.GetTaskProcessor(config[kName].As<std::string>(fmt::format("{}-task-processor", kName)))},
      _director{
        config["topic"].As<std::string>(),
        context.FindComponent<userver::kafka::ProducerComponent>().GetProducer()
    } {
    LOG_INFO() << "Director component start successfully with topic: " << config["topic"].As<std::string>();
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
    task-processor:
        type: string
        description: Task processor for async operations
)");
}

DirectorComponent::~DirectorComponent() {  }


DirectorComponent::ProcessTransactionCallableType DirectorComponent::GetProcessTransactionCallable() const {
    return [this](transaction::Transaction&& transaction) {
        LOG_INFO() << "Director start transaction processing";
        {
            std::unique_lock<userver::engine::Mutex> lk {_u_mx};
            _director.ProcessTransaction(std::move(transaction), _task_processor);
        }
    };
}

DirectorComponent::UpdateProfilesCallableType DirectorComponent::GetUpdateProfilesCallable() {
    return [this](Director::ProfileContainer profiles) {
        LOG_INFO() << "Director start profile updating";
        {
            std::unique_lock<userver::engine::Mutex> lk {_u_mx};
            _director.UpdateProfiles(std::move(profiles));
        }
        LOG_INFO() << "Director end profile updating";
    };
}


} // namespace director_service