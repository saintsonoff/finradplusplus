add_library(composite_rule
    composite_rule.hpp
    composite_rule.cpp
)

target_include_directories(composite_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(composite_rule PUBLIC
    IRule
    rule-config-proto
    rule_utils
)
