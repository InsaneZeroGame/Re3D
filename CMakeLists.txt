cmake_minimum_required(VERSION 3.10)
project(Re3D)
set (CMAKE_CXX_STANDARD 17)

include(cmake/CPM.cmake)

CPMAddPackage(
        NAME GSL
        GITHUB_REPOSITORY "microsoft/GSL"
        GIT_TAG v4.0.0
)

add_subdirectory(game)


