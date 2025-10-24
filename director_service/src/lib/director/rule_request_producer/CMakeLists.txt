find_package(
    userver
COMPONENTS
    kafka
REQUIRED
)

add_library(rule_request_producer STATIC rule_request_producer.cpp)

target_include_directories(rule_request_producer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    rule_request_producer
PUBLIC
    rule_request-proto
PUBLIC
    userver::kafka
)