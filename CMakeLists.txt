cmake_minimum_required(VERSION 3.10)

project(
    telegram-fuse
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

add_definitions(-D_FILE_OFFSET_BITS=64)

add_subdirectory(bin)
add_subdirectory(lib)

