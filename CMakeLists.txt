cmake_minimum_required(VERSION 3.10)
project(Re3D)
set (CMAKE_CXX_STANDARD 20)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
SET(CMAKE_CXX_FLAGS "/wd4996  /EHsc")
#SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")

include(cmake/CPM.cmake)

CPMAddPackage(
        NAME GSL
        GITHUB_REPOSITORY "microsoft/GSL"
        GIT_TAG v4.0.0
)

CPMAddPackage(
        NAME DirectXMesh
        GITHUB_REPOSITORY "microsoft/DirectXMesh"
        GIT_TAG sep2024
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
  NAME EnTT
  GIT_TAG v3.13.2
  GITHUB_REPOSITORY skypjack/entt
  # EnTT's CMakeLists screws with configuration options
  DOWNLOAD_ONLY True
)

if (EnTT_ADDED)
  add_library(EnTT INTERFACE)
  target_include_directories(EnTT INTERFACE ${EnTT_SOURCE_DIR}/src)
endif()

add_subdirectory(engine)
add_subdirectory(renderer)
add_subdirectory(game)
add_subdirectory(game_dxr)


