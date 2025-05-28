cmake_minimum_required(VERSION 3.5)

set(COVERAGE OFF CACHE BOOL "Coverage")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(TestRunning)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/googletest" "gtest")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/banking")
add_executable(RunTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

if(COVERAGE)
    target_compile_options(RunTest PRIVATE --coverage)
    target_link_libraries(RunTest PRIVATE --coverage)
endif()
target_include_directories(RunTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/banking
)
target_link_libraries(RunTest PRIVATE
    gtest
    gtest_main
    gmock_main
    banking
)
