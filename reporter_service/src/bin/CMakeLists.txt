find_package(
    userver
COMPONENTS
    core
REQUIRED
)

add_executable(reporter_service main.cpp)

target_link_libraries(
    reporter_service
PUBLIC
    userver::core
PUBLIC
    rule_result_consumer
)