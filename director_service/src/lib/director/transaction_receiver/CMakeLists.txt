find_package(
    userver
COMPONENTS
    grpc
REQUIRED
)

add_library(transaction_receiver STATIC transaction_receiver.cpp)


target_include_directories(transaction_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
    transaction_receiver
PUBLIC
    transaction-proto
PUBLIC
    userver::grpc
)