cmake_minimum_required(VERSION 4.1.1)
project(rules_service VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(
    userver
    COMPONENTS grpc
    REQUIRED
)

add_subdirectory(src)
