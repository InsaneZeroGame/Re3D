set(TARGET Game_dxr)

include_directories(${CMAKE_SOURCE_DIR}/engine)



set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.cpp pch.h)

include_directories(${CMAKE_SOURCE_DIR}/renderer)

add_executable(${TARGET} WIN32 game_dxr.cpp ${${TARGET}_Headers} ${${TARGET}_Srcs})
target_link_libraries(${TARGET} GSL Renderer spdlog EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)
