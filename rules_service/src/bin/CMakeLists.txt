add_executable(rules_service main.cpp)

target_link_libraries(rules_service PUBLIC transaction_proto ml_proto IRule ml_rule rule_config_proto rule_factory)
