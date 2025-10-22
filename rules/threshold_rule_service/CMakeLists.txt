cmake_minimum_required(VERSION 4.1.1)
project(threshold_rule_service CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHARED_PROTO_DIR "${CMAKE_SOURCE_DIR}/../../shared_data_model/src/proto")

set(CONAN_PROTOBUF_ROOT "$ENV{HOME}/.conan2/p/b")
file(GLOB PROTOBUF_PACKAGE_DIRS "${CONAN_PROTOBUF_ROOT}/proto*/p")
if(PROTOBUF_PACKAGE_DIRS)
    list(GET PROTOBUF_PACKAGE_DIRS 0 PROTOBUF_PACKAGE_DIR)
    set(Protobuf_PROTOC_EXECUTABLE "${PROTOBUF_PACKAGE_DIR}/bin/protoc" CACHE FILEPATH "The protoc compiler" FORCE)
    message(STATUS "Setting protoc from Conan: ${Protobuf_PROTOC_EXECUTABLE}")
endif()

find_package(Protobuf REQUIRED)

set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_PROTOC_EXECUTABLE})

message(STATUS "Using protoc: ${PROTOBUF_PROTOC_EXECUTABLE}")

if(NOT PROTOBUF_PROTOC_EXECUTABLE)
    message(FATAL_ERROR "protoc executable not found from Conan package")
endif()

set(PROTO_FILES
    "${SHARED_PROTO_DIR}/transaction.proto"
    "${SHARED_PROTO_DIR}/rules/threshold_rule.proto"
)

set(PROTO_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/src/proto")
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR})
file(MAKE_DIRECTORY ${PROTO_OUTPUT_DIR}/rules)

set(PROTO_SRCS "")
set(PROTO_HDRS "")

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)

    file(RELATIVE_PATH PROTO_REL_DIR ${SHARED_PROTO_DIR} ${PROTO_DIR})

    if(PROTO_REL_DIR STREQUAL "")
        set(OUTPUT_DIR ${PROTO_OUTPUT_DIR})
    else()
        set(OUTPUT_DIR ${PROTO_OUTPUT_DIR}/${PROTO_REL_DIR})
    endif()

    set(PROTO_SRC "${OUTPUT_DIR}/${PROTO_NAME}.pb.cc")
    set(PROTO_HDR "${OUTPUT_DIR}/${PROTO_NAME}.pb.h")

    list(APPEND PROTO_SRCS ${PROTO_SRC})
    list(APPEND PROTO_HDRS ${PROTO_HDR})

    add_custom_command(
        OUTPUT ${PROTO_SRC} ${PROTO_HDR}
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
        ARGS --cpp_out=${PROTO_OUTPUT_DIR}
             --proto_path=${SHARED_PROTO_DIR}
             ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ code from ${PROTO_FILE}"
        VERBATIM
    )
endforeach()

add_library(threshold_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(threshold_proto PUBLIC
    ${PROTO_OUTPUT_DIR}
)
target_link_libraries(threshold_proto PUBLIC
    protobuf::libprotobuf
)

add_subdirectory(src)
