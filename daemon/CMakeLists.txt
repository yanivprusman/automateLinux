cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)

include_directories(include ${SQLITE3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${JSONCPP_INCLUDE_DIRS} ${LIBEVDEV_INCLUDE_DIRS})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/sendKeys.c") # Add sendKeys.c
add_executable(daemon ${SOURCES})
target_link_libraries(daemon sqlite3 systemd ${CURL_LIBRARIES} ${JSONCPP_LIBRARIES} ${LIBEVDEV_LIBRARIES})
target_compile_definitions(daemon PRIVATE DAEMON_MODE) # Define DAEMON_MODE
add_custom_command(TARGET daemon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:daemon> ${CMAKE_SOURCE_DIR}/daemon
)