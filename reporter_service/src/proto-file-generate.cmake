find_package(
    userver 
    COMPONENTS grpc
    REQUIRED
)
set(PROTO_FILE_PATH "${DATA_PROTO_MODELS_PATH}")

userver_add_grpc_library(rule_result-proto PROTOS "${PROTO_FILE_PATH}/rules/rule_result.proto" SOURCE_PATH "${PROTO_FILE_PATH}")