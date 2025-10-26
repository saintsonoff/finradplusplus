#pragma once

// stdcpp
#include <functional>
#include <string_view>
#include <string>
#include <unordered_set>

//userver
#include <userver/components/component_base.hpp>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/kafka/producer.hpp>
#include <userver/yaml_config/schema.hpp>
#include <userver/engine/mutex.hpp>

// self
#include <transaction/transaction.pb.h>
#include <rules/profile.pb.h>


namespace std {


template<>
struct hash<profile::Profile> {
    std::size_t operator()(const profile::Profile& profile) const {
        return std::hash<std::string>{}(profile.uuid());
    }
};


} // namespace std

namespace director_service {


class Director {
private:
    struct ProfileEqualComparator {
        bool operator()(const profile::Profile& lhs, const profile::Profile& rhs) const {
            return lhs.uuid() == rhs.uuid();
        }
    };

public:
    using ProfileContainer = std::unordered_set<profile::Profile, std::hash<profile::Profile>, ProfileEqualComparator>;

public:
    Director(
        std::string topic,
        const userver::kafka::Producer& producer
    );

public:
    void ProcessTransaction(
        transaction::Transaction&& transaction,
        userver::engine::TaskProcessor& _task_processor
    ) const;
    void UpdateProfiles(ProfileContainer profiles);

private:
    std::string _topic;
    const userver::kafka::Producer& _producer;
    ProfileContainer _profiles;
};


class DirectorComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "director-producer";

    DirectorComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    ~DirectorComponent() override;

public:
    using ProcessTransactionCallableType = std::function<void(transaction::Transaction&&)>;
    using UpdateProfilesCallableType = std::function<void(Director::ProfileContainer)>;

public:
    ProcessTransactionCallableType GetProcessTransactionCallable() const;
    UpdateProfilesCallableType GetUpdateProfilesCallable();

private:
    userver::engine::TaskProcessor& _task_processor;
    Director _director;
    mutable userver::engine::Mutex _u_mx;
};


} // namespace director_service