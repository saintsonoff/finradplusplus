cmake_minimum_required(VERSION 4.1)
project(director_service)

get_filename_component(DATA_MODELS_PATH "$ENV{SHARED_MODELS_PATH}/shared_data_model/src/proto" ABSOLUTE)

include(proto-file-generate.cmake)

add_subdirectory(lib)
add_subdirectory(bin)