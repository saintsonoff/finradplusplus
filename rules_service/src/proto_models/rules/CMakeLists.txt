add_library(ml_proto
    ml_rule.pb.cc
    ml_rule.pb.h
)

add_library(composite_proto
    composite_rule.pb.cc
    composite_rule.pb.h
)

add_library(pattern_proto
    pattern_rule.pb.cc
    pattern_rule.pb.h
)

add_library(threshold_proto
    threshold_rule.pb.cc
    threshold_rule.pb.h
)

add_library(rule_config_proto
    rule_config.pb.cc
    rule_config.pb.h
)

target_include_directories(ml_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(composite_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pattern_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(threshold_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(rule_config_proto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(threshold_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message)
target_link_libraries(pattern_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message)
target_link_libraries(ml_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message)
target_link_libraries(composite_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message)

target_link_libraries(rule_config_proto PUBLIC
    protobuf::libprotobuf absl::log_internal_check_op
    absl::log_internal_message
    threshold_proto pattern_proto ml_proto composite_proto)
