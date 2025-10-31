cmake_minimum_required(VERSION 3.10)
project(termcontrol VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_library(termcontrol_lib
    src/terminal.cpp
    src/buffer.cpp
)

# Include directories
target_include_directories(termcontrol_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add executables
add_executable(tailwindow
    examples/tailwindow.cpp
)

add_executable(termcontrol
    src/main.cpp
)

# Link executables with the library
target_link_libraries(tailwindow PRIVATE termcontrol_lib)
target_link_libraries(termcontrol PRIVATE termcontrol_lib)