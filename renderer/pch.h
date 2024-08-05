//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#pragma once

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4238) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4239) // A non-const reference may only be bound to an lvalue; assignment operator takes a reference to non-const
#pragma warning(disable:4324) // structure was padded due to __declspec(align())
#pragma warning(disable:4996) // structure was padded due to __declspec(align())

#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <d3d12.h>
#include <SimpleMath.h>
#include <dxgi1_6.h>
#include "d3dx12.h"
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include <d3dcompiler.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)
#define MY_IID_PPV_ARGS                     IID_PPV_ARGS


#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <memory>
#include <string>
#include <cwctype>
#include <exception>
#include <fstream>
#include <span>

#include <ppltasks.h>
#include <functional>
#include <string>

#include <gsl/gsl>
#include <gsl/assert>
#include <memory>
#include <spdlog/spdlog.h>
#include <optional>
#include "color.h"
#include <filesystem>
#include <taskflow/taskflow.hpp>
#include <ResourceUploadBatch.h>
#include <GeometricPrimitive.h>
#include "device_manager.h"
#include "gpu_resource.h"
#include "render_utils.h"
#include "graphics_common.h"
#include <entt/entt.hpp>
#include <entt/entity/registry.hpp>
#include <Keyboard.h>
#include <Mouse.h>
#include "renderer_context.h"
#include "PostProcess.h"
#include <engine.h>
#include "renderer_common.h"


#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxguid.lib")