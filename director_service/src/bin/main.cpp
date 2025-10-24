// userver
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>

#include <userver/kafka/producer_component.hpp>

#include <profile_receiver.hpp>
#include <transaction_receiver.hpp>
#include <director.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(ugrpc::server::MinimalComponentList())
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<kafka::ProducerComponent>()
            .Append<director_service::DirectorComponent>()
            .Append<director_service::ProfileReceiverComponent>()
            .Append<director_service::TransactionReceiverComponent>()
        ;
            
    return utils::DaemonMain(argc, argv, component_list);
}