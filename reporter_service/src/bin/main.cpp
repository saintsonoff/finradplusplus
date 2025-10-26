// userver
#include <userver/utils/daemon_run.hpp>

#include <userver/congestion_control/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>


// self
#include <rule_result_consumer.hpp>

int main(int argc, char* argv[]) {
    const auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<reporter_service::RuleResultConsumerComponent>()
        .Append<userver::kafka::ConsumerComponent>("kafka-consumer")
    ;
            
    return userver::utils::DaemonMain(argc, argv, component_list);
}