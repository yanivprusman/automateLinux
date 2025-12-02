
b(){
    ./build.sh "$@"
}
export -f b

m(){
    build/main "$@"
}
export -f m

prepareBuild() {
    local language=""
    local project_dir=""
    if [[ $# -lt 1 ]]; then
        echo "Usage: prepareBuild [-c|-cpp] [directory]"
        echo "  -c   : Create a C project (C11 standard)"
        echo "  -cpp : Create a C++ project (C++17 standard)"
        echo "  directory : Optional target directory (default: current directory)"
        return 1
    fi
    language="$1"
    project_dir="${2:-.}"
    case "$language" in
        -c)
            language="C"
            ;;
        -cpp)
            language="CXX"
            ;;
        *)
            echo "Error: Invalid language option '$language'"
            echo "Use -c for C or -cpp for C++"
            return 1
            ;;
    esac
    local project_name
    if [[ "$project_dir" == "." ]]; then
        project_name=$(basename "$(pwd)")
    else
        project_name=$(basename "$project_dir")
    fi
    if [[ ! -d "$project_dir" ]]; then
        mkdir -p "$project_dir"
    fi
    mkdir -p "$project_dir/include"
    mkdir -p "$project_dir/src"
    mkdir -p "$project_dir/build"
    local cmake_file="$project_dir/CMakeLists.txt"
    if [[ "$language" == "C" ]]; then
        cat > "$cmake_file" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(ProjectName C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(ProjectName ${SOURCES})
EOF
    elif [[ "$language" == "CXX" ]]; then
        cat > "$cmake_file" << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(ProjectName CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic")
find_package(SQLite3 REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/include ${SQLITE3_INCLUDE_DIRS})
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
add_executable(ProjectName ${SOURCES})
target_link_libraries(ProjectName sqlite3 systemd)
EOF
    fi
    sed -i "s/ProjectName/$project_name/g" "$cmake_file"
    local main_file
    if [[ "$language" == "C" ]]; then
        main_file="$project_dir/src/main.c"
        cat > "$main_file" << 'EOF'
#include <stdio.h>
int main(int argc, char *argv[]) {
    (void)argc;    (void)argv;
    printf("Hello from %s\n", "ProjectName");
    return 0;
}
EOF
        sed -i "s/ProjectName/$project_name/g" "$main_file"
    elif [[ "$language" == "CXX" ]]; then
        main_file="$project_dir/src/main.cpp"
        cat > "$main_file" << 'EOF'
#include <iostream>
int main(int argc, char *argv[]) {
    (void)argc;    (void)argv;
    std::cout << "Hello from ProjectName" << std::endl;
    return 0;
}
EOF
        sed -i "s/ProjectName/$project_name/g" "$main_file"
    fi
    local build_script="$project_dir/build.sh"
    cat > "$build_script" << 'EOF'
#!/bin/bash
set -e
if [[ " $@ " =~ " -rebuild " ]]; then
    rm -rf build
fi
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. > /dev/null && \
make > /dev/null && \
echo -e "${GREEN}Build complete!${NC}" 
cd ..
EOF
    chmod +x "$build_script"
}

export -f prepareBuild
