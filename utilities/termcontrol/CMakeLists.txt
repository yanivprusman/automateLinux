cmake_minimum_required(VERSION 3.10)
project(termcontrol VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
add_library(termcontrol
    src/terminal.cpp
    src/buffer.cpp
)

# Include directories
target_include_directories(termcontrol PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Add example executable
add_executable(example
    examples/main.cpp
)

# Link example with the library
target_link_libraries(example PRIVATE termcontrol)