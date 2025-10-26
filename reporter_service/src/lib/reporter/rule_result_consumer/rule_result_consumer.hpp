#pragma once


// cppstd
#include <string_view>


// userver
#include <userver/components/component_base.hpp>

#include <userver/kafka/consumer_scope.hpp>
#include <userver/kafka/message.hpp>


// models
#include <rules/rule_result.pb.h>


namespace reporter_service {


struct RuleResultConsumer {
    void operator()(userver::kafka::MessageBatchView messages) const;
};

inline constexpr RuleResultConsumer kRuleResultConsumer = {};


class RuleResultConsumerComponent final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "rule-result-consumer";

    RuleResultConsumerComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    );

private:
    userver::kafka::ConsumerScope _consumer;
};


} // namespace reporter_service