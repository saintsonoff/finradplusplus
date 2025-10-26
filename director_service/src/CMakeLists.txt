cmake_minimum_required(VERSION 4.1)
project(director_service)

get_filename_component(DATA_PROTO_MODELS_PATH "$ENV{DIRECTOR_SHARED_MODELS_PATH}/src/proto" ABSOLUTE)

include(proto-file-generate.cmake)

add_subdirectory(lib)
add_subdirectory(bin)