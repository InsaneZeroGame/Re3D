set(TARGET Engine)

if(NOT COMMAND fbx_target_finalize)
    # Architecture detection
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM64")
            SET(FBX_ARM 1)
            set(FBX_ARCH arm64) # 64 bits
        else()
            set(FBX_ARCH x64) # 64 bits
        endif()
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        if("${CMAKE_C_COMPILER_ARCHITECTURE_ID}" STREQUAL "ARM")
            SET(FBX_ARM 1)
            set(FBX_ARCH arm) # 32 bits
        else()
            set(FBX_ARCH x86) # 32 bits
        endif()
    endif()

    if(NOT FBX_CONFIGURATION)
        if(WIN32)
            set(FBX_CONFIGURATION vc${MSVC_TOOLSET_VERSION}_${FBX_ARCH}_dll)
        else()
            if(APPLE)
                set(FBX_CONFIGURATION clang_macos_dylib)
            else()
                set(FBX_CONFIGURATION gcc_${FBX_ARCH}_so_lib)
            endif()
        endif()
    endif()

    if(NOT FBX_BUILD_ROOT)
        set(FBX_BUILD_ROOT ${CMAKE_SOURCE_DIR}/../build/${FBX_CONFIGURATION})
    endif()
    IF(NOT EXISTS "${FBX_BUILD_ROOT}")
        FILE(MAKE_DIRECTORY "${FBX_BUILD_ROOT}")
    endif()

    INCLUDE("fbxsdk.cmake")
endif()

if(FBX_SHARED)
    add_compile_definitions(FBXSDK_SHARED)
endif()


CPMAddPackage(
        NAME DirectXTK12
        GITHUB_REPOSITORY "microsoft/DirectXTK12"
        GIT_TAG feb2024
)

CPMAddPackage(
        NAME spdlog
        GITHUB_REPOSITORY "gabime/spdlog"
        GIT_TAG v1.13.0
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
            game_scene.h
            fbx_loader.h
            logger.h
            engine.h
)

set(${TARGET}_Srcs 
            game_scene.cpp
            components.cpp
            delegates.cpp
            SimpleMath.cpp
            camera.cpp
            asset_loader.cpp
            stb_texture_loader.cpp
            graphics_common.cpp
            utility.cpp
            obj_model_loader.cpp
            fbx_loader.cpp  
            logger.cpp    
            engine.cpp
)

set(${TARGET}_Srcs
            ${${TARGET}_Srcs}
            pch.h pch.cpp)

add_library(${TARGET} ${${TARGET}_Headers} ${${TARGET}_Srcs} )
target_link_libraries(${TARGET} GSL spdlog glfw DirectXTK12 EnTT fbxsdk)
fbx_target_finalize(${TARGET})
target_precompile_headers(${TARGET} PRIVATE pch.h)