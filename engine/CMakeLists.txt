set(TARGET Engine)

CPMAddPackage(
        NAME DirectXTK12
        GITHUB_REPOSITORY "microsoft/DirectXTK12"
        GIT_TAG feb2024
)

set(${TARGET}_Headers
            delegates.h
            camera.h
            SimpleMath.h
            SimpleMath.inl
            graphics_common.h
            asset_loader.h
            obj_model_loader.h
            stb_image.h
            stb_texture_loader.h
            utility.h
            tiny_obj_loader.h
            components.h
)

set(${TARGET}_Srcs 
            components.cpp
            delegates.cpp
            SimpleMath.cpp
            camera.cpp
            asset_loader.cpp
            stb_texture_loader.cpp
            graphics_common.cpp
            utility.cpp
            obj_model_loader.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.h pch.cpp)

add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs} )
target_link_libraries(${TARGET} GSL spdlog glfw DirectXTK12 EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)