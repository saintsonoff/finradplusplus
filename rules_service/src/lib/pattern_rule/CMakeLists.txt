add_library(pattern_rule
    pattern_rule.cpp
    pattern_rule.hpp
)

target_link_libraries(pattern_rule PUBLIC IRule pattern_proto rule_config_proto)

target_include_directories(pattern_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
