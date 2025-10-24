add_library(rule_processor STATIC
    rule_processor.cpp
    rule_processor.hpp
)

target_link_libraries(rule_processor
    PUBLIC
        userver::kafka
        userver::redis
        rule_factory
        rule-request-proto
        rule-response-proto
        transaction_history
)

target_include_directories(rule_processor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_BINARY_DIR}/proto
)
