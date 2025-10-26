#include <userver/clients/dns/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/kafka/consumer_component.hpp>
#include <userver/kafka/producer_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include "rule_processor/rule_processor.hpp"

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    auto component_list = userver::components::MinimalServerComponentList()
        .Append<userver::clients::dns::Component>()
        .Append<userver::components::Secdist>()
        .Append<userver::components::DefaultSecdistProvider>()
        .Append<userver::components::TestsuiteSupport>()
        .Append<userver::components::Postgres>("postgres-db-1")
        .Append<userver::kafka::ConsumerComponent>()
        .Append<userver::kafka::ProducerComponent>()
        .Append<fraud_detection::RuleProcessor>();

    return userver::utils::DaemonMain(argc, argv, component_list);
}
