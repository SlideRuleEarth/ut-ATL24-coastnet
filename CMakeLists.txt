cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ATL24_resnet VERSION 1.0.0 LANGUAGES CXX)

find_package(OpenMP REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Torch REQUIRED)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

message(STATUS "Torch version ${Torch_VERSION}")

set(CMAKE_CXX_FLAGS "-Wall -Werror ${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

include_directories(${PROJECT_SOURCE_DIR} viper/ ../ATL24_utils)

############################################################
# Unit tests
############################################################
macro(add_test name)
    add_executable(${name} ./tests/${name}.cpp)
    target_link_libraries(${name} ${TORCH_LIBRARIES} gmp)
endmacro()

add_test(test_pgm)
add_test(test_gpu)

############################################################
# Applications
############################################################

add_executable(latlong_to_atm ./apps/latlong_to_atm.cpp)
target_link_libraries(latlong_to_atm gmp gdal)

add_executable(train ./apps/train.cpp)
target_link_libraries(train ${TORCH_LIBRARIES} gmp ${OpenCV_LIBS})
target_include_directories(train PUBLIC ${OpenCV_INCLUDE_DIRS})
target_precompile_headers(train PUBLIC ATL24_resnet/precompiled.h)

add_executable(classifier ./apps/classifier.cpp)
target_link_libraries(classifier ${TORCH_LIBRARIES} gmp ${OpenCV_LIBS})
target_include_directories(classifier PUBLIC ${OpenCV_INCLUDE_DIRS})
target_precompile_headers(classifier PUBLIC ATL24_resnet/precompiled.h)
