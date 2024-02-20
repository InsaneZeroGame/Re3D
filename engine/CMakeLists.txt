set(TARGET Engine)

set(${TARGET}_Headers
            camera.h
            SimpleMath.h
            SimpleMath.inl
            graphics_common.h
            asset_loader.h
            obj_model_loader.h
            utility.h
            tiny_obj_loader.h)

set(${TARGET}_Srcs 
            SimpleMath.cpp
            camera.cpp
            asset_loader.cpp
            graphics_common.cpp
            utility.cpp
            obj_model_loader.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.h pch.cpp)

add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs} )
target_link_libraries(${TARGET} GSL spdlog)
target_precompile_headers(${TARGET} PRIVATE pch.h)