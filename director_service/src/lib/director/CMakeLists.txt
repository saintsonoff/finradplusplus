add_subdirectory(profile_receiver)
add_subdirectory(transaction_receiver)
add_subdirectory(rule_request_producer)

find_package(
    userver
COMPONENTS
    kafka
REQUIRED
)

add_library(director STATIC director.cpp)

target_include_directories(director PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    director
PUBLIC
    userver::kafka
PUBLIC
    rule_request_producer
PUBLIC
    transaction-proto
    rule_profile-proto
    rule_config-proto
)