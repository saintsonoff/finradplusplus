add_subdirectory(rules)

add_library(transaction_proto
    transaction.pb.cc
    transaction.pb.h
)

target_include_directories(transaction_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(transaction_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message)
