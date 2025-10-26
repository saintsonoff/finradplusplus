#pragma once

// libstd
#include "director.hpp"
#include <string>
#include <string_view>

// userver
#include <userver/ugrpc/server/service_component_base.hpp>

// userver utils
#include <fmt/format.h>

// models
#include <transaction/transaction.pb.h>
#include <transaction/transaction_service.usrv.pb.hpp>

// self
#include <director.hpp>


namespace director_service {


class TransactionReceiver final : public transaction::TransactionServiceBase {
public:
    explicit TransactionReceiver(
        std::string prefix,
        DirectorComponent::ProcessTransactionCallableType process_callable
    );

    ProcessTransactionResult ProcessTransaction(CallContext&, transaction::Transaction&& request) override;

    ~TransactionReceiver() override;

private:
    const std::string _prefix;
    DirectorComponent::ProcessTransactionCallableType _process_callable;
};



class TransactionReceiverComponent final : public userver::ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "transaction-receiver-service";

    TransactionReceiverComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    ~TransactionReceiverComponent() override = default;

private:
    TransactionReceiver _service;
};


} // namespace director_service