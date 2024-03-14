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

CPMAddPackage(
        NAME GLFW
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.3.2
        OPTIONS
          "GLFW_BUILD_TESTS OFF"
          "GLFW_BUILD_EXAMPLES OFF"
          "GLFW_BULID_DOCS OFF"
)

CPMAddPackage(
        NAME taskflow
        GITHUB_REPOSITORY "taskflow/taskflow"
        GIT_TAG v3.6.0
)

CPMAddPackage(
        NAME entt
        GITHUB_REPOSITORY "skypjack/entt"
        GIT_TAG v3.13.1
)

add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(game)


