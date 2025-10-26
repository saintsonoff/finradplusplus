add_library(transaction_history STATIC
    transaction_history_service.cpp
)

target_include_directories(transaction_history PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(transaction_history
    PUBLIC
        userver::postgresql
        transaction-proto
)
