add_subdirectory(profile_receiver)
add_subdirectory(transaction_receiver)

# find_package(
#     userver
# COMPONENTS
#     grpc
# REQUIRED
# )

# add_library(director STATIC director.cpp)

# target_link_libraries(
#     director
# PUBLIC
#     userver::grpc
# PUBLIC
#     transaction-proto
#     rule_config-proto
#     rule_profile-proto
# )