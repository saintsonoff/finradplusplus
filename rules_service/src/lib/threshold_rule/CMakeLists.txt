add_library(threshold_rule
    threshold_rule.cpp
    threshold_rule.hpp
)

target_link_libraries(threshold_rule PUBLIC IRule threshold_proto rule_config_proto)

target_include_directories(threshold_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
