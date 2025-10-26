find_package(
    userver
COMPONENTS
    kafka
REQUIRED
)

add_library(rule_result_consumer STATIC rule_result_consumer.cpp)
target_include_directories(rule_result_consumer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
    rule_result_consumer
PUBLIC
    rule_result-proto
PUBLIC
    userver::kafka
)