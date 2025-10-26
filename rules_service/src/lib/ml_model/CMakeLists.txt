project(ml_model CXX)

# Найти библиотеки XGBoost и LightGBM
find_library(XGBOOST_LIB NAMES xgboost libxgboost PATHS /usr/local/lib /usr/lib /opt/homebrew/lib)
find_library(LIGHTGBM_LIB NAMES _lightgbm lib_lightgbm lightgbm PATHS 
    /usr/local/lib 
    /usr/lib 
    /opt/homebrew/lib
    /usr/local/lib/python3.10/dist-packages/lightgbm/lib
)

if(NOT XGBOOST_LIB)
    message(FATAL_ERROR "XGBoost library not found. Install with: apt-get install libxgboost-dev")
endif()

if(NOT LIGHTGBM_LIB)
    message(WARNING "LightGBM library not found. Stage 1 model will be disabled. Install with: pip3 install lightgbm")
endif()

# Найти заголовочные файлы
find_path(XGBOOST_INCLUDE NAMES xgboost/c_api.h PATHS /usr/local/include /usr/include /opt/homebrew/include)
find_path(LIGHTGBM_INCLUDE NAMES LightGBM/c_api.h PATHS 
    /usr/local/include 
    /usr/include 
    /opt/homebrew/include
)

add_library(${PROJECT_NAME} STATIC
    ml_fraud_detector.cpp
    ml_fraud_detector.hpp
    redis_history_provider.cpp
    redis_history_provider.hpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${XGBOOST_INCLUDE}
)

if(LIGHTGBM_INCLUDE)
    target_include_directories(${PROJECT_NAME} PUBLIC ${LIGHTGBM_INCLUDE})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_LIGHTGBM=1)
    message(STATUS "LightGBM support enabled")
else()
    message(STATUS "LightGBM support disabled (headers not found)")
endif()

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        transaction-proto
        userver-core
        userver-redis
    PRIVATE
        transaction_history
        ${XGBOOST_LIB}
)

if(LIGHTGBM_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${LIGHTGBM_LIB})
endif()
