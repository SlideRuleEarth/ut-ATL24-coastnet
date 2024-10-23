cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ATL24_coastnet VERSION 1.0.0 LANGUAGES CXX)

find_package(OpenMP REQUIRED)
find_package(xgboost REQUIRED)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(CMAKE_CXX_FLAGS "-Wall -Werror -Wshadow ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/ATL24_coastnet)

add_executable(classify ./apps/classify.cpp)
target_link_libraries(classify xgboost::xgboost)
target_precompile_headers(classify PUBLIC ATL24_coastnet/precompiled.h)

