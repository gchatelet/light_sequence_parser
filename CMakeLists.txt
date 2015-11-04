cmake_minimum_required(VERSION 2.8.11)

set(LSP_PROJECT "sequenceparser")

file(GLOB_RECURSE LSP_SRC_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
file(GLOB_RECURSE LSP_HDR_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} include/*.hpp;include/*.h)

add_library(${LSP_PROJECT} ${LSP_SRC_FILES} ${LEONARDOQT_HDR_FILES} ${LSP_HDR_FILES})

set_property(TARGET ${LSP_PROJECT} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_property(TARGET ${LSP_PROJECT} PROPERTY COMPILE_FLAGS -std=c++0x)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(USE_TESTS)

enable_testing()
find_package(GTest REQUIRED)

file(GLOB_RECURSE LSP_TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
add_executable(${LSP_PROJECT}_test ${LSP_TEST_FILES})

#include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
target_link_libraries(${LSP_PROJECT}_test ${GTEST_MAIN_LIBRARIES} ${GTEST_LIBRARIES})

add_test(${LSP_PROJECT} ${LSP_PROJECT}_test)

endif(USE_TESTS)
