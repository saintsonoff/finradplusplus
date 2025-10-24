// userver
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/ugrpc/server/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/kafka/consumer_component.hpp>

#include <profile_receiver.hpp>
#include <transaction_receiver.hpp>

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(ugrpc::server::MinimalComponentList())
            .Append<director_service::ProfileReceiverComponent>()
            .Append<director_service::TransactionReceiverComponent>()
        ;
            
    return utils::DaemonMain(argc, argv, component_list);
}