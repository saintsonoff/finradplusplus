#include "profile_receiver.hpp"

// self
#include <rules/profile.pb.h>
#include <unordered_set>

// userver
#include <userver/logging/log.hpp> 

// models
#include <rules/profile_service.usrv.pb.hpp>
#include <google/protobuf/empty.pb.h>

// self
#include <director.hpp>

namespace director_service {


ProfileReceiver::ProfileReceiver(std::string prefix, DirectorComponent::UpdateProfilesCallableType update_callable) 
    : _prefix(prefix), _update_callable(std::move(update_callable)) {  };

ProfileReceiver::ProcessProfileStreamResult ProfileReceiver::ProcessProfileStream(
  CallContext&,
  ProcessProfileStreamReader& reader) {
    profile::Profile request;
    LOG_INFO() << "Profile receiver stream connection open";

    Director::ProfileContainer profiles;
    while (reader.Read(request)) {
        LOG_INFO() << fmt::format("receive new profile, name: {} uuid: {}", request.name(), request.uuid());
        profiles.insert(std::move(request));
    }
    LOG_INFO() << "Profile receiver stream connection closed";

    _update_callable(std::move(profiles));

    google::protobuf::Empty response;
    return response;
}

ProfileReceiver::~ProfileReceiver() {  }


ProfileReceiverComponent::ProfileReceiverComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context
) 
    : userver::ugrpc::server::ServiceComponentBase(config, context),
    _service(
        config["profile-prefix"].As<std::string>(),
        context.FindComponent<DirectorComponent>("director-producer").GetUpdateProfilesCallable()
    ) {
    RegisterService(_service);
}

userver::yaml_config::Schema ProfileReceiverComponent::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::ugrpc::server::ServiceComponentBase>(R"(
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