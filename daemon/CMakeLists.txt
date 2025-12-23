cmake_minimum_required(VERSION 3.10)
project(HelloWorld)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wall -Wextra -Wpedantic")
find_package(SQLite3 REQUIRED)
include_directories(include ${SQLITE3_INCLUDE_DIRS})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/src/sendKeys.c") # Add sendKeys.c
add_executable(main ${SOURCES})
target_link_libraries(main sqlite3 systemd)
target_compile_definitions(main PRIVATE DAEMON_MODE) # Define DAEMON_MODE
add_custom_command(TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:main> ${CMAKE_SOURCE_DIR}/daemon
)