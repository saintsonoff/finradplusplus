add_library(threshold_rule
    threshold_rule.cpp
    threshold_rule.hpp
)

target_link_libraries(threshold_rule PUBLIC IRule rule-config-proto rule_utils)

target_include_directories(threshold_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
