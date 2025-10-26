#include "transaction_receiver.hpp"

// userver
#include <transaction/transaction.pb.h>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/logging/log.hpp> 

// protobuf
#include <google/protobuf/empty.pb.h>

// self
#include <transaction/transaction_service.usrv.pb.hpp>

namespace director_service {


TransactionReceiver::TransactionReceiver(std::string prefix) 
    : _prefix(prefix) {  };

TransactionReceiver::ProcessTransactionResult TransactionReceiver::ProcessTransaction(
  CallContext&, 
  transaction::Transaction&& request) {
      LOG_INFO() << fmt::format("receive new transaction, id: {}", request.transaction_id());
      google::protobuf::Empty response;
    return response;
}

TransactionReceiver::~TransactionReceiver() {  }


TransactionReceiverComponent::TransactionReceiverComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : ugrpc::server::ServiceComponentBase(config, context), _service(config["transaction-prefix"].As<std::string>()) {
    RegisterService(_service);
}

yaml_config::Schema TransactionReceiverComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
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