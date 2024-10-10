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
            light_cull_pass.h
            base_renderer.h
            renderer_dxr.h
            mesh_shader_pass.h
            )

set(${TARGET}_Srcs 
            gui.cpp
            render_utils.cpp
            skybox.cpp
            render_pass.cpp
            gpu_resource.cpp
            device_manager.cpp
            renderer_context.cpp
            renderer.cpp
            light_cull_pass.cpp
            base_renderer.cpp
            renderer_dxr.cpp
            mesh_shader_pass.cpp
)

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
shaders/SimplePS.hlsl
shaders/MeshShader.hlsl
shaders/AmplifyShader.hlsl
)
source_group("Shaders" FILES ${HLSL_SHADER_FILES})
# Copy shaders to a certain path
set(SHADERS_DESTINATION ${CMAKE_BINARY_DIR}/bin/shaders)
file(COPY ${HLSL_SHADER_FILES} DESTINATION ${SHADERS_DESTINATION})



add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs} ${IMGUI} )
target_link_libraries(${TARGET} GSL spdlog Engine Taskflow EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)
add_custom_target(Shaders SOURCES ${HLSL_SHADER_FILES})


