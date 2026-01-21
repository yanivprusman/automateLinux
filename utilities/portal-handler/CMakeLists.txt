cmake_minimum_required(VERSION 3.10)
project(portal-handler)

set(CMAKE_CXX_STANDARD 17)

# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Find GIO (for portal/session management)
pkg_check_modules(GIO REQUIRED gio-2.0 gio-unix-2.0)

include_directories(
    ${GIO_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Shared Library
add_library(portalhandler_lib SHARED
    src/PortalManager.cpp
)

target_link_libraries(portalhandler_lib
    ${GIO_LIBRARIES}
    stdc++fs
)

target_include_directories(portalhandler_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Executable (CLI tool)
add_executable(portal-handler src/main.cpp)

target_link_libraries(portal-handler
    portalhandler_lib
)
