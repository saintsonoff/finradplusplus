find_package(
    userver
COMPONENTS
    grpc
REQUIRED
)

add_library(profile_receiver STATIC profile_receiver.cpp)


target_include_directories(profile_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    profile_receiver
PUBLIC
    rule_profile-proto
PUBLIC
    userver::grpc
)