find_package(
    userver 
    COMPONENTS grpc
    REQUIRED
)

set(PROTO_FILE_PATH "${DATA_MODELS_PATH}/transaction")
userver_add_grpc_library(transaction-proto PROTOS "${PROTO_FILE_PATH}/transaction.proto" SOURCE_PATH "${PROTO_FILE_PATH}")

set(PROTO_FILE_PATH "${DATA_MODELS_PATH}/rules")
userver_add_grpc_library(rule_config-proto PROTOS "${PROTO_FILE_PATH}/rule_config.proto" SOURCE_PATH "${PROTO_FILE_PATH}")
userver_add_grpc_library(rule_profile-proto PROTOS "${PROTO_FILE_PATH}/profile.proto" SOURCE_PATH "${PROTO_FILE_PATH}")
target_link_libraries(rule_profile-proto PUBLIC rule_config-proto)