cmake_minimum_required(VERSION 4.1)
project(director_service)

get_filename_component(DATA_MODELS_PATH "${CMAKE_SOURCE_DIR}/../../shared_data_model/src/proto" ABSOLUTE)

add_subdirectory(bin)