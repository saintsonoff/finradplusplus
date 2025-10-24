#pragma once

// stdcpp
#include <functional>
#include <string_view>
#include <string>
#include <unordered_set>

//userver
#include <userver/components/component_base.hpp>

#include <userver/kafka/producer.hpp>
#include <userver/yaml_config/schema.hpp>

// self
#include <transaction/transaction.pb.h>
#include <rules/profile.pb.h>

bool operator==(const profile::Profile& lhs, const profile::Profile& rhs);

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
public:
    Director(const userver::kafka::Producer& producer, std::string topic);

    void ProcessTransaction(transaction::Transaction transaction);
    void RefreshProfiles(std::unordered_set<profile::Profile> profiles);

private:

private:
    std::string _topic;
    const userver::kafka::Producer& _producer;
    std::unordered_set<profile::Profile> _profiles;
};


class DirectorComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "director-producer";

    DirectorComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

    static userver::yaml_config::Schema GetStaticConfigSchema();

    ~DirectorComponent() override = default;

private:
    Director _director;
};


} // namespace director_service