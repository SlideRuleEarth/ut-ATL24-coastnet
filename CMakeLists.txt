cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ATL24_coastnet VERSION 1.0.0 LANGUAGES CXX)

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

include_directories(${PROJECT_SOURCE_DIR})

############################################################
# Unit tests
############################################################
macro(add_test name)
    add_executable(${name} ./tests/${name}.cpp)
    target_link_libraries(${name} ${TORCH_LIBRARIES} gmp)
endmacro()

add_test(test_pgm)
add_test(test_gpu)
add_test(test_blunder_detection)

############################################################
# Applications
############################################################

add_executable(train ./apps/train.cpp)
target_link_libraries(train ${TORCH_LIBRARIES} gmp ${OpenCV_LIBS})
target_include_directories(train PUBLIC ${OpenCV_INCLUDE_DIRS})
target_precompile_headers(train PUBLIC ATL24_coastnet/precompiled.h)

add_executable(classify ./apps/classify.cpp)
target_link_libraries(classify ${TORCH_LIBRARIES} gmp)
target_include_directories(classify PUBLIC)
target_precompile_headers(classify PUBLIC ATL24_coastnet/precompiled.h)

add_executable(train_coastnet ./apps/train_coastnet.cpp)
target_link_libraries(train_coastnet ${TORCH_LIBRARIES} gmp)
target_include_directories(train_coastnet PUBLIC)
target_precompile_headers(train_coastnet PUBLIC ATL24_coastnet/precompiled.h)

add_executable(classify_coastnet ./apps/classify_coastnet.cpp)
target_link_libraries(classify_coastnet ${TORCH_LIBRARIES} gmp)
target_include_directories(classify_coastnet PUBLIC)
target_precompile_headers(classify_coastnet PUBLIC ATL24_coastnet/precompiled.h)

add_executable(score_coastnet ./apps/score_coastnet.cpp)
target_link_libraries(score_coastnet ${TORCH_LIBRARIES} gmp)
target_include_directories(score_coastnet PUBLIC)
target_precompile_headers(score_coastnet PUBLIC ATL24_coastnet/precompiled.h)

add_executable(blunder_detection ./apps/blunder_detection.cpp)
target_link_libraries(blunder_detection ${TORCH_LIBRARIES} gmp)
target_include_directories(blunder_detection PUBLIC)
target_precompile_headers(blunder_detection PUBLIC ATL24_coastnet/precompiled.h)

add_executable(combine_predictions ./apps/combine_predictions.cpp)
target_link_libraries(combine_predictions ${TORCH_LIBRARIES} gmp)
target_include_directories(combine_predictions PUBLIC)
target_precompile_headers(combine_predictions PUBLIC ATL24_coastnet/precompiled.h)
