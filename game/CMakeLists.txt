set(TARGET triangle)

CPMAddPackage(
        NAME GLFW
        GITHUB_REPOSITORY glfw/glfw
        GIT_TAG 3.3.2
        OPTIONS
          "GLFW_BUILD_TESTS OFF"
          "GLFW_BUILD_EXAMPLES OFF"
          "GLFW_BULID_DOCS OFF"
)

set(${TARGET}_Headers 
            window.h)

set(${TARGET}_Srcs 
            window.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.cpp pch.h)


add_executable(${TARGET} triangle.cpp ${${TARGET}_Headers} ${${TARGET}_Srcs})
target_link_libraries(${TARGET} glfw GSL)
target_precompile_headers(${TARGET} PRIVATE pch.h)
