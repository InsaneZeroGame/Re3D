set(TARGET triangle)

include_directories(${CMAKE_SOURCE_DIR}/engine)



set(${TARGET}_Headers 
            window.h)

set(${TARGET}_Srcs 
            window.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.cpp pch.h)

include_directories(${CMAKE_SOURCE_DIR}/renderer)

add_executable(${TARGET} WIN32 triangle.cpp ${${TARGET}_Headers} ${${TARGET}_Srcs})
target_link_libraries(${TARGET} glfw GSL Renderer spdlog EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)
