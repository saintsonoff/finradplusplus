#pragma once

// libstd
#include <string>
#include <string_view>

// userver
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

// userver utils
#include <fmt/format.h>

// self
#include <rules/profile_service.usrv.pb.hpp>


namespace director_service {


class ProfileReceiver final : public profile::ProfileServiceBase {
public:
    explicit ProfileReceiver(std::string prefix);

    ProcessProfileStreamResult ProcessProfileStream(CallContext&, ProcessProfileStreamReader& reader) override;

    ~ProfileReceiver() override;
private:
    const std::string _prefix;
};



class ProfileReceiverComponent final : public ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "profile-receiver-service";

    ProfileReceiverComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

static yaml_config::Schema GetStaticConfigSchema();

    ~ProfileReceiverComponent() override = default;
private:
    ProfileReceiver _service;
};


} // namespace director_service