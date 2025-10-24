add_library(ml_rule
    ml_rule.cpp
    ml_rule.hpp
)

target_link_libraries(ml_rule PUBLIC IRule rule-config-proto)

target_include_directories(ml_rule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
