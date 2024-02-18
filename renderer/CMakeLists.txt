set(TARGET Renderer)

set(${TARGET}_Headers
            color.h
            gpu_resource.h
            d3dx12.h
            device_manager.h
            renderer.h)

set(${TARGET}_Srcs 
            gpu_resource.cpp
            device_manager.cpp
            renderer.cpp)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.cpp pch.h)

set(HLSL_SHADER_FILES shaders/ForwardVS.hlsl shaders/ForwardPS.hlsl)
set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE shaders/ForwardVS.hlsl PROPERTY VS_SHADER_MODEL 5.0)
set_property(SOURCE shaders/ForwardPS.hlsl PROPERTY VS_SHADER_MODEL 5.0)
set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_DISABLE_OPTIMIZATIONS true)
set_property(SOURCE ${HLSL_SHADER_FILES} PROPERTY VS_SHADER_ENABLE_DEBUG true)




add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs} ${HLSL_SHADER_FILES})
target_link_libraries(${TARGET} GSL spdlog)
target_precompile_headers(${TARGET} PRIVATE pch.h)


