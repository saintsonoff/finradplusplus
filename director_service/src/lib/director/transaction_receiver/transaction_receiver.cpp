#include "transaction_receiver.hpp"

// userver
#include <userver/logging/log.hpp> 
#include <userver/components/component_context.hpp>
#include <userver/components/component_base.hpp>

// protobuf
#include <google/protobuf/empty.pb.h>

// models
#include <transaction/transaction_service.usrv.pb.hpp>
#include <transaction/transaction.pb.h>

// self
#include <director.hpp>

namespace director_service {


TransactionReceiver::TransactionReceiver(
    std::string prefix,
    DirectorComponent::ProcessTransactionCallableType process_callable
) 
    : _prefix(prefix), _process_callable(process_callable) {  };

TransactionReceiver::ProcessTransactionResult TransactionReceiver::ProcessTransaction(
  CallContext&, 
  transaction::Transaction&& request) {
    LOG_INFO() << fmt::format("receive new transaction, id: {}", request.transaction_id());
    _process_callable(std::move(request));
    google::protobuf::Empty response;
    return response;
}

TransactionReceiver::~TransactionReceiver() {  }


TransactionReceiverComponent::TransactionReceiverComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) 
  : userver::ugrpc::server::ServiceComponentBase(config, context),
  _service(
        config["transaction-prefix"].As<std::string>(),
        context.FindComponent<DirectorComponent>("director-producer").GetProcessTransactionCallable()
  ) {
    RegisterService(_service);
}

userver::yaml_config::Schema TransactionReceiverComponent::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC profile get service component
additionalProperties: false
properties:
    transaction-prefix:
        type: string
        description: transaction prefix
)");
}


} // namespace director_service