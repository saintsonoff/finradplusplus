# rule_utils - header-only library with shared utilities

add_library(rule_utils INTERFACE)

target_include_directories(rule_utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(rule_utils INTERFACE
    rule-config-proto
    transaction-proto
)
