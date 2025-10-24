#pragma once

// stdcpp
#include <string_view>

//userver
#include <userver/components/component_base.hpp>

#include <userver/kafka/producer.hpp>
#include <userver/yaml_config/schema.hpp>


namespace director_service {


class Director {
public:
    Director(const userver::kafka::Producer& producer);

private:
    const userver::kafka::Producer& _producer;  
};


class DirectorComponent final : public userver::components::LoggableComponentBase {
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