cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
# MySQL Connector/C++
find_library(MYSQLCPPCONN_LIBRARY NAMES mysqlcppconn PATHS /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
find_path(MYSQLCPPCONN_INCLUDE_DIR NAMES mysql_connection.h PATHS /usr/include /usr/local/include)

if (MYSQLCPPCONN_LIBRARY AND MYSQLCPPCONN_INCLUDE_DIR)
    message(STATUS "Found MySQL Connector/C++: ${MYSQLCPPCONN_LIBRARY}")
    include_directories(${MYSQLCPPCONN_INCLUDE_DIR})
else()
    message(FATAL_ERROR "MySQL Connector/C++ not found")
endif()

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# JSONCpp
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

# libevdev
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

include_directories(
    include
    ${JSONCPP_INCLUDE_DIRS}
    ${LIBEVDEV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/sendKeys.c") # Add sendKeys.c
add_executable(daemon ${SOURCES})
# Link libraries
target_link_libraries(daemon
    ${MYSQLCPPCONN_LIBRARY}
    ${JSONCPP_LIBRARIES}
    ${LIBEVDEV_LIBRARIES}
    systemd
    ${CURL_LIBRARIES}
    ${Boost_LIBRARIES}
    Threads::Threads
    curl
    pthread
    stdc++fs
)
target_compile_definitions(daemon PRIVATE DAEMON_MODE) # Define DAEMON_MODE
add_custom_command(TARGET daemon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:daemon> ${CMAKE_SOURCE_DIR}/daemon
)