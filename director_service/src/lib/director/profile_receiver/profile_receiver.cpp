#include "profile_receiver.hpp"

// userver
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/logging/log.hpp> 

// protobuf
#include <google/protobuf/empty.pb.h>

// self
#include <profile_service.usrv.pb.hpp>

namespace director_service {


ProfileReceiver::ProfileReceiver(std::string prefix) 
    : _prefix(prefix) {  };

ProfileReceiver::ProcessProfileStreamResult ProfileReceiver::ProcessProfileStream(
  CallContext&,
  ProcessProfileStreamReader& reader) {
    profile::Profile request;
    LOG_INFO() << fmt::format("profile receiver stream connection open");
    while (reader.Read(request)) {
        LOG_INFO() << fmt::format("receive new profile, name: {} uuid: {}", request.name(), request.uuid());
    }
    LOG_INFO() << fmt::format("profile receiver stream connection closed");

    google::protobuf::Empty response;
    return response;
}

ProfileReceiver::~ProfileReceiver() {  }


ProfileReceiverComponent::ProfileReceiverComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) : ugrpc::server::ServiceComponentBase(config, context), _service(config["profile-prefix"].As<std::string>()) {
    RegisterService(_service);
}

yaml_config::Schema ProfileReceiverComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC profile get service component
additionalProperties: false
properties:
    profile-prefix:
        type: string
        description: profile
)");
}


} // namespace director_service