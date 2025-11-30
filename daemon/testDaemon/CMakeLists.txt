cmake_minimum_required(VERSION 3.10)
project(UnixSocketExample C)
set(CMAKE_C_STANDARD 11)
include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB TEST_DAEMON_SRC ${CMAKE_SOURCE_DIR}/src/*.c)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(testDaemon ${TEST_DAEMON_SRC})


