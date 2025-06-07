cmake_minimum_required(VERSION 3.10)
project(ArenaBattle LANGUAGES CXX)

# Use C++20 and enable strict warnings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# add_compile_options(-Wall -Wextra -Werror -pedantic)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/
)

# Gather all .cpp files under src/ and common/
file(GLOB SRC_SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/common/*.cpp"
)

# Build the executable “tanks_game”
add_executable(tanks_game
    ${SRC_SOURCES}
)