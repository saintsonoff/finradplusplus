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
#include <profile_service.usrv.pb.hpp>
#include <google/protobuf/empty.pb.h>


namespace director_service {


class ProfileReceiver final : public profile::ProfileServiceBase {
public:
    explicit ProfileReceiver(std::string prefix) : _prefix(prefix) {  };

    ProcessProfileStreamResult ProcessProfileStream(CallContext&, ProcessProfileStreamReader& reader) override {
        profile::Profile request;
        while (reader.Read(request)) {
            LOG_INFO() << fmt::format("receive new profile, name: {} uuid: {}", request.name(), request.uuid());
        }

        google::protobuf::Empty response;
        return response;
    }

    ~ProfileReceiver() override {  }
private:
    const std::string _prefix;
};



class ProfileReceiverComponent final : public ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "profile-service";

    ProfileReceiverComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    ) : ugrpc::server::ServiceComponentBase(config, context), _service(config["profile-prefix"].As<std::string>()) {
        RegisterService(_service);
    }

static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC profile get service component
additionalProperties: false
properties:
    profile-prefix:
        type: string
        description: profile prefix
)");
}

    ~ProfileReceiverComponent() override {  }
private:
    ProfileReceiver _service;
};


} // namespace director_service