add_library(IRule INTERFACE)

target_link_libraries(IRule INTERFACE
    transaction-proto)

target_include_directories(IRule INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
