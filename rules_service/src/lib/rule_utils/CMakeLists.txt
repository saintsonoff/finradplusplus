add_library(rule_utils
    kafka_result_producer.cpp
)

target_include_directories(rule_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(rule_utils PUBLIC
    rule-config-proto
    transaction-proto
    userver::kafka
)
