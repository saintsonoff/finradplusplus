add_library(rule_factory INTERFACE)

target_include_directories(rule_factory INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(rule_factory INTERFACE 
IRule 
ml_rule
pattern_rule 
threshold_rule 
composite_rule 
rule_config_proto
)