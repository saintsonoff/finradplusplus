add_library(pattern_rule
    pattern_rule.cpp
    pattern_rule.hpp
)

target_link_libraries(pattern_rule PUBLIC IRule rule-config-proto rule_utils transaction_history)

target_include_directories(pattern_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
