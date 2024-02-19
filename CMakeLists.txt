cmake_minimum_required(VERSION 3.10)
project(Re3D)
set (CMAKE_CXX_STANDARD 17)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(cmake/CPM.cmake)

CPMAddPackage(
        NAME GSL
        GITHUB_REPOSITORY "microsoft/GSL"
        GIT_TAG v4.0.0
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY "gabime/spdlog"
        GIT_TAG v1.13.0
)
add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(game)


