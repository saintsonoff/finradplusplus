#pragma once

// libstd
#include <string>
#include <string_view>

// userver
#include <userver/ugrpc/server/service_component_base.hpp>

// another
#include <fmt/format.h>

// self
#include <director.hpp>

// models
#include <rules/profile_service.usrv.pb.hpp>


namespace director_service {


class ProfileReceiver final : public profile::ProfileServiceBase {
public:
    explicit ProfileReceiver(std::string prefix, DirectorComponent::UpdateProfilesCallableType update_callable);

    ProcessProfileStreamResult ProcessProfileStream(CallContext&, ProcessProfileStreamReader& reader) override;

    ~ProfileReceiver() override;

private:
    const std::string _prefix;
    DirectorComponent::UpdateProfilesCallableType _update_callable;
};



class ProfileReceiverComponent final : public userver::ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "profile-receiver-service";

    ProfileReceiverComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    ~ProfileReceiverComponent() override = default;

private:
    ProfileReceiver _service;
};


} // namespace director_service