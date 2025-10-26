find_package(
    userver
COMPONENTS
    kafka
    core
REQUIRED
)

add_library(rule_request_producer STATIC rule_request_producer.cpp)

target_include_directories(rule_request_producer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    rule_request_producer
PUBLIC
    transaction-proto
    rule_profile-proto
    rule_config-proto
    rule_request-proto
PUBLIC
    userver::kafka
)

add_library(director STATIC director.cpp)

target_include_directories(director PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    director
PUBLIC
    rule_request_producer
PUBLIC
    transaction-proto
    rule_profile-proto
    rule_config-proto
PUBLIC
    userver::core
)