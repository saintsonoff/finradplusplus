find_package(
    userver 
    COMPONENTS grpc
    REQUIRED
)
set(PROTO_FILE_PATH "${DATA_MODELS_PATH}")

userver_add_grpc_library(transaction-proto PROTOS "${PROTO_FILE_PATH}/transaction/transaction.proto" SOURCE_PATH "${PROTO_FILE_PATH}")

userver_add_grpc_library(rule_config-proto PROTOS "${PROTO_FILE_PATH}/rules/rule_config.proto" SOURCE_PATH "${PROTO_FILE_PATH}")

userver_add_grpc_library(rule_profile-proto PROTOS "${PROTO_FILE_PATH}/rules/profile.proto" SOURCE_PATH "${PROTO_FILE_PATH}")
target_link_libraries(rule_profile-proto PUBLIC rule_config-proto)

userver_add_grpc_library(rule_request-proto PROTOS "${PROTO_FILE_PATH}/rules/rule_request.proto" SOURCE_PATH "${PROTO_FILE_PATH}")
target_link_libraries(
    rule_request-proto
PUBLIC
    transaction-proto
    rule_config-proto
)