#pragma once

// libstd
#include <string>
#include <string_view>

// userver
#include <transaction.pb.h>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

// userver utils
#include <fmt/format.h>

// self
#include <transaction_service.usrv.pb.hpp>


namespace director_service {


class TransactionReceiver final : public transaction::TransactionServiceBase {
public:
    explicit TransactionReceiver(std::string prefix);

    ProcessTransactionResult ProcessTransaction(CallContext&, transaction::Transaction&& request) override;

    ~TransactionReceiver() override;
private:
    const std::string _prefix;
};



class TransactionReceiverComponent final : public ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "transaction-receiver-service";

    TransactionReceiverComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

static yaml_config::Schema GetStaticConfigSchema();

    ~TransactionReceiverComponent() override = default;
private:
    TransactionReceiver _service;
};


} // namespace director_service