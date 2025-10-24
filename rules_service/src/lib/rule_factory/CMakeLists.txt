add_library(rule_factory STATIC rule_factory.cpp)

target_include_directories(rule_factory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(rule_factory PUBLIC
    IRule 
    rule-config-proto
    threshold_rule
    composite_rule
    ml_rule
    pattern_rule
    transaction_history
)