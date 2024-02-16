set(TARGET Renderer)

set(${TARGET}_Headers
            d3dx12.h
            device_manager.h
            renderer.h)

set(${TARGET}_Srcs 
            device_manager.cpp
            renderer.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.cpp pch.h)


add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs})
target_link_libraries(${TARGET} GSL)
target_precompile_headers(${TARGET} PRIVATE pch.h)
