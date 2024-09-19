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
)
set_property(SOURCE shaders/SkyboxVS.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/SkyboxPS.hlsl PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE shaders/LightCull.hlsl PROPERTY VS_SHADER_TYPE Compute)
set_property(SOURCE shaders/ShadowMap.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/SimplePS.hlsl PROPERTY VS_SHADER_TYPE Pixel)

set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/LightCull.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/SkyboxVS.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/SkyboxPS.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/ShadowMap.hlsl PROPERTY VS_SHADER_MODEL 6.5)
set_property(SOURCE shaders/SimplePS.hlsl PROPERTY VS_SHADER_MODEL 6.5)


# Set the path to the DXC executable
set(DXC_PATH "${PROJECT_SOURCE_DIR}/renderer/shaders/DXC/bin/x64/dxc.exe")

# Get the current configuration type
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-configuration generator (e.g., Visual Studio)
    set(CONFIGURATION "$<CONFIG>")
else()
    # Single-configuration generator (e.g., Makefile)
    set(CONFIGURATION "${CMAKE_BUILD_TYPE}")
endif()

# Function to compile HLSL shaders
function(compile_shader SHADER_FILE ENTRY_POINT TARGET_PROFILE OUTPUT_FILE)
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${DXC_PATH} -T ${TARGET_PROFILE} -E ${ENTRY_POINT} -Fo ${OUTPUT_FILE} -Zi -Qembed_debug ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling ${SHADER_FILE} to ${OUTPUT_FILE}"
    )
endfunction()

# Compile the mesh shader
message("Compiling mesh shader")
set(SHADER_FILE "${PROJECT_SOURCE_DIR}/renderer/shaders/MeshShader.hlsl")
set(MESH_SHADER_OUTPUT "${CMAKE_BINARY_DIR}/${CONFIGURATION}/MeshShader.cso")
compile_shader(${SHADER_FILE} "ms_main" "ms_6_5" ${MESH_SHADER_OUTPUT})

if(DEBUG)
    set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_DISABLE_OPTIMIZATIONS true)
    set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_ENABLE_DEBUG true)
endif()
add_library(${TARGET} ${HLSL_SHADER_FILES} ${${TARGET}_Headers} ${${TARGET}_Srcs} ${IMGUI} ${MESH_SHADER_OUTPUT})
target_link_libraries(${TARGET} GSL spdlog Engine Taskflow EnTT)
target_precompile_headers(${TARGET} PRIVATE pch.h)


