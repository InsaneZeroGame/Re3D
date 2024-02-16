#include "device_manager.h"

#ifdef WIN32
#include <winreg.h>		// To read the registry
#endif

constexpr static bool RequireDXRSupport = true;
static bool g_bTypedUAVLoadSupport_R11G11B10_FLOAT = false;
static bool g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = false;
constexpr static DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
constexpr static int SWAP_CHAIN_BUFFER_COUNT = 3;

// Check adapter support for DirectX Raytracing.
bool IsDirectXRaytracingSupported(ID3D12Device* testDevice)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport = {};

	if (FAILED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport, sizeof(featureSupport))))
		return false;

	return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

Renderer::DeviceManager::DeviceManager()
{
	CreateD3DDevice();
	CreateCmdManager();
}

void Renderer::DeviceManager::CreateD3DDevice()
{
	Microsoft::WRL::ComPtr<ID3D12Device> pDevice;

	uint32_t useDebugLayers = 0;
#if _DEBUG
	// Default to true for debug builds
	useDebugLayers = 1;
#endif

	DWORD dxgiFactoryFlags = 0;

	if (useDebugLayers)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(MY_IID_PPV_ARGS(&debugInterface))))
		{
			debugInterface->EnableDebugLayer();

			uint32_t useGPUBasedValidation = 0;
#if _DEBUG
			useGPUBasedValidation = 1;
#endif
			if (useGPUBasedValidation)
			{
				Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
				if (SUCCEEDED((debugInterface->QueryInterface(MY_IID_PPV_ARGS(&debugInterface1)))))
				{
					debugInterface1->SetEnableGPUBasedValidation(true);
				}
			}
		}
		else
		{
			//Utility::Print("WARNING:  Unable to enable D3D12 debug validation layer\n");
		}

#if _DEBUG
		IDXGIInfoQueue* dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
		{
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

			DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
			{
				80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
			};
			DXGI_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
		}
#endif
	}

	// Obtain the DXGI factory
	Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory;
	Ensures(CreateDXGIFactory2(dxgiFactoryFlags, MY_IID_PPV_ARGS(&dxgiFactory)) == S_OK);

	// Create the D3D graphics device
	Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter;

	uint32_t bUseWarpDriver = false;

	// Temporary workaround because SetStablePowerState() is crashing
	D3D12EnableExperimentalFeatures(0, nullptr, nullptr, nullptr);

	if (!bUseWarpDriver)
	{
		SIZE_T MaxSize = 0;

		for (uint32_t Idx = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
		{
			DXGI_ADAPTER_DESC1 desc;
			pAdapter->GetDesc1(&desc);

			// Is a software adapter?
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			
			// Can create a D3D12 device?
			if (FAILED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, MY_IID_PPV_ARGS(&pDevice))))
				continue;

			// Does support DXR if required?
			if (RequireDXRSupport && !IsDirectXRaytracingSupported(pDevice.Get()))
				continue;

			// By default, search for the adapter with the most memory because that's usually the dGPU.
			if (desc.DedicatedVideoMemory < MaxSize)
				continue;

			MaxSize = desc.DedicatedVideoMemory;

			if (mDevice != nullptr)
				mDevice->Release();

			mDevice = pDevice.Detach();
		}
	}

	if (RequireDXRSupport && !mDevice)
	{
		__debugbreak();
	}

	if (mDevice == nullptr)
	{
		Ensures(dxgiFactory->EnumWarpAdapter(MY_IID_PPV_ARGS(&pAdapter)));
		Ensures(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_11_0, MY_IID_PPV_ARGS(&pDevice)) == S_OK);
		mDevice = pDevice.Detach();
	}
#ifndef RELEASE
	else
	{
		bool DeveloperModeEnabled = false;

		// Look in the Windows Registry to determine if Developer Mode is enabled
		HKEY hKey;
		LSTATUS result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_READ, &hKey);
		if (result == ERROR_SUCCESS)
		{
			DWORD keyValue, keySize = sizeof(DWORD);
			result = RegQueryValueEx(hKey, "AllowDevelopmentWithoutDevLicense", 0, NULL, (byte*)&keyValue, &keySize);
			if (result == ERROR_SUCCESS && keyValue == 1)
				DeveloperModeEnabled = true;
			RegCloseKey(hKey);
		}
		Ensures(DeveloperModeEnabled);
		// Prevent the GPU from over clocking or under clocking to get consistent timings
		if (DeveloperModeEnabled)
			mDevice->SetStablePowerState(TRUE);
	}
#endif	

#if _DEBUG
	ID3D12InfoQueue* pInfoQueue = nullptr;
	if (SUCCEEDED(mDevice->QueryInterface(MY_IID_PPV_ARGS(&pInfoQueue))))
	{
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] =
		{
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			(D3D12_MESSAGE_ID)1008,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		pInfoQueue->PushStorageFilter(&NewFilter);
		pInfoQueue->Release();
	}
#endif

	// We like to do read-modify-write operations on UAVs during post processing.  To support that, we
	// need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
	// decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
	// load support.
	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
	if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData))))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT Support =
			{
				DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE
			};

			if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
				(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				g_bTypedUAVLoadSupport_R11G11B10_FLOAT = true;
			}

			Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

			if (SUCCEEDED(mDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support, sizeof(Support))) &&
				(Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0)
			{
				g_bTypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
			}
		}
	}
}



Renderer::DeviceManager::~DeviceManager()
{

}

void Renderer::DeviceManager::SetTargetWindow(HWND InWindow, int InWidth, int InHeight)
{
	mWidth = InWidth;
	mHeight = InHeight;
	mWindow = InWindow;
	CreateSwapChain();
}

void Renderer::DeviceManager::CreateSwapChain()
{
	Expects(s_SwapChain1 == nullptr);

	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Ensures(CreateDXGIFactory2(0, MY_IID_PPV_ARGS(&dxgiFactory)) == S_OK);

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = SwapChainFormat;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	Ensures(dxgiFactory->CreateSwapChainForHwnd(
		mCmdManager->GetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
		mWindow,
		&swapChainDesc,
		&fsSwapChainDesc,
		nullptr,
		&s_SwapChain1) == S_OK);

#if CONDITIONALLY_ENABLE_HDR_OUTPUT
	{
		IDXGISwapChain4* swapChain = (IDXGISwapChain4*)s_SwapChain1;
		ComPtr<IDXGIOutput> output;
		ComPtr<IDXGIOutput6> output6;
		DXGI_OUTPUT_DESC1 outputDesc;
		UINT colorSpaceSupport;

		// Query support for ST.2084 on the display and set the color space accordingly
		if (SUCCEEDED(swapChain->GetContainingOutput(&output)) && SUCCEEDED(output.As(&output6)) &&
			SUCCEEDED(output6->GetDesc1(&outputDesc)) && outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 &&
			SUCCEEDED(swapChain->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport)) &&
			(colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
			SUCCEEDED(swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
		{
			g_bEnableHDROutput = true;
		}
	}
#endif // End CONDITIONALLY_ENABLE_HDR_OUTPUT

	for (uint32_t i = 0; i < SWAP_CHAIN_BUFFER_COUNT; ++i)
	{
		ID3D12Resource* DisplayPlane;
		Ensures(s_SwapChain1->GetBuffer(i, IID_PPV_ARGS(&DisplayPlane)) == S_OK);
		//g_DisplayPlane[i].CreateFromSwapChain(L"Primary SwapChain Buffer", DisplayPlane.Detach());
	}
}

void Renderer::DeviceManager::CreateCmdManager()
{
	mCmdManager = std::make_unique<CmdManager>(mDevice);
}

Renderer::CmdManager::CmdManager(ID3D12Device* InDevice):
	mDevice(InDevice)
{
	std::vector<D3D12_COMMAND_LIST_TYPE> types = { 
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_LIST_TYPE_COMPUTE,
		D3D12_COMMAND_LIST_TYPE_COPY };

	for (auto& type : types)
	{
		mAllocatorMap.try_emplace(type, std::make_unique<CommandAllocatorPool>(type));
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = type;
		ID3D12CommandQueue* queue;
		mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue));
		mQueueMap.try_emplace(type, queue);
	}
}

Renderer::CmdManager::~CmdManager()
{
	
}

ID3D12CommandList* Renderer::CmdManager::AllocateCmdList(D3D12_COMMAND_LIST_TYPE InType)
{
	ID3D12CommandAllocator* newAllocator = mAllocatorMap[InType]->RequestAllocator(0);
	ID3D12CommandList* newCmdList;
	mDevice->CreateCommandList(0, InType, newAllocator, nullptr, IID_PPV_ARGS(&newCmdList));
	return newCmdList;
}

ID3D12CommandQueue* Renderer::CmdManager::GetQueue(D3D12_COMMAND_LIST_TYPE InType)
{
	return mQueueMap[InType];
}

Renderer::CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type):
	m_cCommandListType(Type),
	m_Device(nullptr)
{

}

Renderer::CommandAllocatorPool::~CommandAllocatorPool()
{
	Shutdown();
}

void Renderer::CommandAllocatorPool::Create(ID3D12Device* pDevice)
{
	m_Device = pDevice;
}

void Renderer::CommandAllocatorPool::Shutdown()
{
	for (size_t i = 0; i < m_AllocatorPool.size(); ++i)
		m_AllocatorPool[i]->Release();

	m_AllocatorPool.clear();
}

ID3D12CommandAllocator* Renderer::CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
	std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

	ID3D12CommandAllocator* pAllocator = nullptr;

	if (!m_ReadyAllocators.empty())
	{
		std::pair<uint64_t, ID3D12CommandAllocator*>& AllocatorPair = m_ReadyAllocators.front();

		if (AllocatorPair.first <= CompletedFenceValue)
		{
			pAllocator = AllocatorPair.second;
			Ensures(pAllocator->Reset() == S_OK);
			m_ReadyAllocators.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (pAllocator == nullptr)
	{
		Ensures(m_Device->CreateCommandAllocator(m_cCommandListType, MY_IID_PPV_ARGS(&pAllocator)) == S_OK);
		wchar_t AllocatorName[32];
		swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
		pAllocator->SetName(AllocatorName);
		m_AllocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void Renderer::CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator)
{
	std::lock_guard<std::mutex> LockGuard(m_AllocatorMutex);

	// That fence value indicates we are free to reset the allocator
	m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}
