get_filename_component(DATA_MODELS_PATH "${CMAKE_SOURCE_DIR}/../shared_data_model/src/proto" ABSOLUTE)

userver_add_grpc_library(transaction-proto PROTOS "${DATA_MODELS_PATH}/transaction/transaction.proto" SOURCE_PATH "${DATA_MODELS_PATH}")
userver_add_grpc_library(rule-config-proto PROTOS "${DATA_MODELS_PATH}/rules/rule_config.proto" SOURCE_PATH "${DATA_MODELS_PATH}")
userver_add_grpc_library(rule-request-proto PROTOS "${DATA_MODELS_PATH}/requests/rule_request.proto" SOURCE_PATH "${DATA_MODELS_PATH}")
userver_add_grpc_library(rule-response-proto PROTOS "${DATA_MODELS_PATH}/responses/rule_response.proto" SOURCE_PATH "${DATA_MODELS_PATH}")

target_link_libraries(rule-request-proto PUBLIC rule-config-proto transaction-proto)

