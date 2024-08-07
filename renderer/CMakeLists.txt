set(TARGET Renderer)

include_directories(${CMAKE_SOURCE_DIR}/engine)

file(GLOB_RECURSE IMGUI imgui/*.cpp imgui/*.h)

set(${TARGET}_Headers
            render_pass.h
            color.h
            gpu_resource.h
            d3dx12.h
            device_manager.h
            renderer.h
            skybox.h
            render_utils.h
            gui.h
            renderer_context.h
            renderer_common.h
            )

set(${TARGET}_Srcs 
            gui.cpp
            render_utils.cpp
            skybox.cpp
            render_pass.cpp
            gpu_resource.cpp
            device_manager.cpp
            renderer_context.cpp
            renderer.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.h pch.cpp)

set(HLSL_SHADER_FILES 
shaders/ForwardVS.hlsl 
shaders/ForwardPS.hlsl 
shaders/LightCull.hlsl 
shaders/shader_common.hlsli
shaders/SkyboxVS.hlsl
shaders/SkyboxPS.hlsl
shaders/ShadowMap.hlsl
)
set_property(SOURCE shaders/SkyboxVS.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/SkyboxPS.hlsl PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE shaders/LightCull.hlsl PROPERTY VS_SHADER_TYPE Compute)
set_property(SOURCE shaders/ShadowMap.hlsl PROPERTY VS_SHADER_TYPE Vertex)

set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_MODEL 5.1)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_MODEL 5.1)
set_property(SOURCE shaders/LightCull.hlsl PROPERTY VS_SHADER_MODEL 5.1)
set_property(SOURCE shaders/SkyboxVS.hlsl PROPERTY VS_SHADER_MODEL 5.1)
set_property(SOURCE shaders/SkyboxVS.hlsl PROPERTY VS_SHADER_MODEL 5.1)
set_property(SOURCE shaders/ShadowMap.hlsl PROPERTY VS_SHADER_MODEL 5.1)

if(DEBUG)
    set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_DISABLE_OPTIMIZATIONS true)
    set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_ENABLE_DEBUG true)
endif()
add_library(${TARGET} ${HLSL_SHADER_FILES} ${${TARGET}_Headers} ${${TARGET}_Srcs} ${IMGUI})
target_link_libraries(${TARGET} GSL spdlog Engine Taskflow EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)


