add_library(IRule INTERFACE)

target_link_libraries(IRule INTERFACE
    transaction_proto
    rule_config_proto)

target_include_directories(IRule INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
