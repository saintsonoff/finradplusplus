add_executable(rules_service main.cpp)

target_link_libraries(rules_service PUBLIC
    userver::kafka
    userver::redis
    userver::core
    transaction-proto
    rule-config-proto
    rule-request-proto
    rule-result-proto
    result-service-proto
    rule_factory
    rule_processor
)
