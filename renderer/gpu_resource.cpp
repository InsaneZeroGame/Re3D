#include "gpu_resource.h"
#include "color.h"
#include "device_manager.h"

namespace Renderer
{
	namespace Resource
	{

		DXGI_FORMAT PixelBuffer::GetBaseFormat(DXGI_FORMAT defaultFormat)
		{
			switch (defaultFormat)
			{
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return DXGI_FORMAT_R8G8B8A8_TYPELESS;

			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return DXGI_FORMAT_B8G8R8A8_TYPELESS;

			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return DXGI_FORMAT_B8G8R8X8_TYPELESS;

				// 32-bit Z w/ Stencil
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return DXGI_FORMAT_R32G8X24_TYPELESS;

				// No Stencil
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
				return DXGI_FORMAT_R32_TYPELESS;

				// 24-bit Z
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return DXGI_FORMAT_R24G8_TYPELESS;

				// 16-bit Z w/o Stencil
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
				return DXGI_FORMAT_R16_TYPELESS;

			default:
				return defaultFormat;
			}
		}

		DXGI_FORMAT PixelBuffer::GetUAVFormat(DXGI_FORMAT defaultFormat)
		{
			switch (defaultFormat)
			{
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return DXGI_FORMAT_R8G8B8A8_UNORM;

			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
				return DXGI_FORMAT_B8G8R8A8_UNORM;

			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return DXGI_FORMAT_B8G8R8X8_UNORM;

			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_R32_FLOAT:
				return DXGI_FORMAT_R32_FLOAT;

#ifdef _DEBUG
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_D16_UNORM:

				assert(false, "Requested a UAV Format for a depth stencil Format.");
#endif

			default:
				return defaultFormat;
			}
		}

		DXGI_FORMAT PixelBuffer::GetDSVFormat(DXGI_FORMAT defaultFormat)
		{
			switch (defaultFormat)
			{
				// 32-bit Z w/ Stencil
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

				// No Stencil
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
				return DXGI_FORMAT_D32_FLOAT;

				// 24-bit Z
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return DXGI_FORMAT_D24_UNORM_S8_UINT;

				// 16-bit Z w/o Stencil
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
				return DXGI_FORMAT_D16_UNORM;

			default:
				return defaultFormat;
			}
		}

		DXGI_FORMAT PixelBuffer::GetDepthFormat(DXGI_FORMAT defaultFormat)
		{
			switch (defaultFormat)
			{
				// 32-bit Z w/ Stencil
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

				// No Stencil
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
				return DXGI_FORMAT_R32_FLOAT;

				// 24-bit Z
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

				// 16-bit Z w/o Stencil
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
				return DXGI_FORMAT_R16_UNORM;

			default:
				return DXGI_FORMAT_UNKNOWN;
			}
		}

		DXGI_FORMAT PixelBuffer::GetStencilFormat(DXGI_FORMAT defaultFormat)
		{
			switch (defaultFormat)
			{
				// 32-bit Z w/ Stencil
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

				// 24-bit Z
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
				return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

			default:
				return DXGI_FORMAT_UNKNOWN;
			}
		}

		//--------------------------------------------------------------------------------------
		// Return the BPP for a particular format
		//--------------------------------------------------------------------------------------
		size_t PixelBuffer::BytesPerPixel(DXGI_FORMAT Format)
		{
			switch (Format)
			{
			case DXGI_FORMAT_R32G32B32A32_TYPELESS:
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
			case DXGI_FORMAT_R32G32B32A32_UINT:
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return 16;

			case DXGI_FORMAT_R32G32B32_TYPELESS:
			case DXGI_FORMAT_R32G32B32_FLOAT:
			case DXGI_FORMAT_R32G32B32_UINT:
			case DXGI_FORMAT_R32G32B32_SINT:
				return 12;

			case DXGI_FORMAT_R16G16B16A16_TYPELESS:
			case DXGI_FORMAT_R16G16B16A16_FLOAT:
			case DXGI_FORMAT_R16G16B16A16_UNORM:
			case DXGI_FORMAT_R16G16B16A16_UINT:
			case DXGI_FORMAT_R16G16B16A16_SNORM:
			case DXGI_FORMAT_R16G16B16A16_SINT:
			case DXGI_FORMAT_R32G32_TYPELESS:
			case DXGI_FORMAT_R32G32_FLOAT:
			case DXGI_FORMAT_R32G32_UINT:
			case DXGI_FORMAT_R32G32_SINT:
			case DXGI_FORMAT_R32G8X24_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
				return 8;

			case DXGI_FORMAT_R10G10B10A2_TYPELESS:
			case DXGI_FORMAT_R10G10B10A2_UNORM:
			case DXGI_FORMAT_R10G10B10A2_UINT:
			case DXGI_FORMAT_R11G11B10_FLOAT:
			case DXGI_FORMAT_R8G8B8A8_TYPELESS:
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			case DXGI_FORMAT_R8G8B8A8_UINT:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
			case DXGI_FORMAT_R8G8B8A8_SINT:
			case DXGI_FORMAT_R16G16_TYPELESS:
			case DXGI_FORMAT_R16G16_FLOAT:
			case DXGI_FORMAT_R16G16_UNORM:
			case DXGI_FORMAT_R16G16_UINT:
			case DXGI_FORMAT_R16G16_SNORM:
			case DXGI_FORMAT_R16G16_SINT:
			case DXGI_FORMAT_R32_TYPELESS:
			case DXGI_FORMAT_D32_FLOAT:
			case DXGI_FORMAT_R32_FLOAT:
			case DXGI_FORMAT_R32_UINT:
			case DXGI_FORMAT_R32_SINT:
			case DXGI_FORMAT_R24G8_TYPELESS:
			case DXGI_FORMAT_D24_UNORM_S8_UINT:
			case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			case DXGI_FORMAT_R8G8_B8G8_UNORM:
			case DXGI_FORMAT_G8R8_G8B8_UNORM:
			case DXGI_FORMAT_B8G8R8A8_UNORM:
			case DXGI_FORMAT_B8G8R8X8_UNORM:
			case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
			case DXGI_FORMAT_B8G8R8A8_TYPELESS:
			case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			case DXGI_FORMAT_B8G8R8X8_TYPELESS:
			case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
				return 4;

			case DXGI_FORMAT_R8G8_TYPELESS:
			case DXGI_FORMAT_R8G8_UNORM:
			case DXGI_FORMAT_R8G8_UINT:
			case DXGI_FORMAT_R8G8_SNORM:
			case DXGI_FORMAT_R8G8_SINT:
			case DXGI_FORMAT_R16_TYPELESS:
			case DXGI_FORMAT_R16_FLOAT:
			case DXGI_FORMAT_D16_UNORM:
			case DXGI_FORMAT_R16_UNORM:
			case DXGI_FORMAT_R16_UINT:
			case DXGI_FORMAT_R16_SNORM:
			case DXGI_FORMAT_R16_SINT:
			case DXGI_FORMAT_B5G6R5_UNORM:
			case DXGI_FORMAT_B5G5R5A1_UNORM:
			case DXGI_FORMAT_A8P8:
			case DXGI_FORMAT_B4G4R4A4_UNORM:
				return 2;

			case DXGI_FORMAT_R8_TYPELESS:
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_R8_UINT:
			case DXGI_FORMAT_R8_SNORM:
			case DXGI_FORMAT_R8_SINT:
			case DXGI_FORMAT_A8_UNORM:
			case DXGI_FORMAT_P8:
				return 1;

			default:
				return 0;
			}
		}

		void PixelBuffer::AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState)
		{
			(Device); // Unused until we support multiple adapters

			Expects(Resource != nullptr);
			D3D12_RESOURCE_DESC ResourceDesc = Resource->GetDesc();

			m_pResource = Resource;
			m_UsageState = CurrentState;

			m_Width = (uint32_t)ResourceDesc.Width;		// We don't care about large virtual textures yet
			m_Height = ResourceDesc.Height;
			m_ArraySize = ResourceDesc.DepthOrArraySize;
			m_Format = ResourceDesc.Format;

#ifndef RELEASE
			m_pResource->SetName(Name.c_str());
#else
			(Name);
#endif
		}

		D3D12_RESOURCE_DESC PixelBuffer::DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize,
			uint32_t NumMips, DXGI_FORMAT Format, UINT Flags)
		{
			m_Width = Width;
			m_Height = Height;
			m_ArraySize = DepthOrArraySize;
			m_Format = Format;

			D3D12_RESOURCE_DESC Desc = {};
			Desc.Alignment = 0;
			Desc.DepthOrArraySize = (UINT16)DepthOrArraySize;
			Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
			Desc.Format = GetBaseFormat(Format);
			Desc.Height = (UINT)Height;
			Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			Desc.MipLevels = (UINT16)NumMips;
			Desc.SampleDesc.Count = 1;
			Desc.SampleDesc.Quality = 0;
			Desc.Width = (UINT64)Width;
			return Desc;
		}

		void PixelBuffer::CreateTextureResource(ID3D12Device* Device, const std::wstring& Name,
			const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
		{
			Destroy();

			(void)VidMemPtr;

			{
				CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
				Ensures(Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
					&ResourceDesc, D3D12_RESOURCE_STATE_COMMON, &ClearValue, MY_IID_PPV_ARGS(&m_pResource)) == S_OK);
			}

			m_UsageState = D3D12_RESOURCE_STATE_COMMON;
			m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

#ifndef RELEASE
			m_pResource->SetName(Name.c_str());
#else
			(Name);
#endif
		}

		void PixelBuffer::ExportToFile(const std::wstring& FilePath)
		{
			assert(0);
			// This very short command list only issues one API call and will be synchronized so we can immediately read
			// the buffer contents.
			//ReadbackBuffer TempBuffer;
			//CommandContext& Context = CommandContext::Begin(L"Copy texture to memory");
			//uint32_t RowPitch = Context.ReadbackTexture(TempBuffer, *this);
			//Context.Finish(true);
			//
			//// Retrieve a CPU-visible pointer to the buffer memory.  Map the whole range for reading.
			//void* Memory = TempBuffer.Map();
			//
			//// Open the file and write the header followed by the texel data.
			//std::ofstream OutFile(FilePath, std::ios::out | std::ios::binary);
			//OutFile.write((const char*)&m_Format, 4);
			//OutFile.write((const char*)&RowPitch, 4);
			//OutFile.write((const char*)&m_Width, 4);
			//OutFile.write((const char*)&m_Height, 4);
			//OutFile.write((const char*)Memory, TempBuffer.GetBufferSize());
			//OutFile.close();
			//
			//// No values were written to the buffer, so use a null range when unmapping.
			//TempBuffer.Unmap();
		}

		void ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
		{
			Expects(ArraySize == 1 || NumMips == 1);
			m_NumMipMaps = NumMips - 1;

			D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
			D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
			D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

			RTVDesc.Format = Format;
			UAVDesc.Format = GetUAVFormat(Format);
			SRVDesc.Format = Format;
			SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (ArraySize > 1)
			{
				RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				RTVDesc.Texture2DArray.MipSlice = 0;
				RTVDesc.Texture2DArray.FirstArraySlice = 0;
				RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

				UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				UAVDesc.Texture2DArray.MipSlice = 0;
				UAVDesc.Texture2DArray.FirstArraySlice = 0;
				UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
				SRVDesc.Texture2DArray.MipLevels = NumMips;
				SRVDesc.Texture2DArray.MostDetailedMip = 0;
				SRVDesc.Texture2DArray.FirstArraySlice = 0;
				SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
			}
			else if (m_FragmentCount > 1)
			{
				RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				RTVDesc.Texture2D.MipSlice = 0;

				UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				UAVDesc.Texture2D.MipSlice = 0;

				SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				SRVDesc.Texture2D.MipLevels = NumMips;
				SRVDesc.Texture2D.MostDetailedMip = 0;
			}

			if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			{
				auto [cpuHandleRTV,gpuHanleRTV] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Allocate();
				m_RTVHandle = cpuHandleRTV;
				auto [cpuHandle, gpuHanle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
				m_SRVHandle = cpuHandle;
			}

			ID3D12Resource* Resource = m_pResource;

			// Create the render target view
			Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

			// Create the shader resource view
			Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

			if (m_FragmentCount > 1)
				return;

			// Create the UAVs for each mip level (RWTexture2D)
			for (uint32_t i = 0; i < NumMips; ++i)
			{
				if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
				{
					auto [cpuHandle, gpuHanle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->Allocate();
					m_UAVHandle[i] = cpuHandle;
				}

				Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

				UAVDesc.Texture2D.MipSlice++;
			}
		}

		void ColorBuffer::CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource)
		{
			AssociateWithResource(Renderer::g_Device, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

			//m_UAVHandle[0] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			//Graphics::g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, nullptr, m_UAVHandle[0]);
			auto [cpuHandle, gpuHanle] = g_DescHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Allocate();
			m_RTVHandle = cpuHandle;
			g_Device->CreateRenderTargetView(m_pResource, nullptr, m_RTVHandle);
		}

		void ColorBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
			DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
		{
			NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
			D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
			D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

			ResourceDesc.SampleDesc.Count = m_FragmentCount;
			ResourceDesc.SampleDesc.Quality = 0;

			D3D12_CLEAR_VALUE ClearValue = {};
			ClearValue.Format = Format;
			ClearValue.Color[0] = m_ClearColor.R();
			ClearValue.Color[1] = m_ClearColor.G();
			ClearValue.Color[2] = m_ClearColor.B();
			ClearValue.Color[3] = m_ClearColor.A();

			CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMem);
			CreateDerivedViews(g_Device, Format, 1, NumMips);
		}
		
		void ColorBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
			DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
		{
			D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
			D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

			D3D12_CLEAR_VALUE ClearValue = {};
			ClearValue.Format = Format;
			ClearValue.Color[0] = m_ClearColor.R();
			ClearValue.Color[1] = m_ClearColor.G();
			ClearValue.Color[2] = m_ClearColor.B();
			ClearValue.Color[3] = m_ClearColor.A();

			CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMem);
			CreateDerivedViews(g_Device, Format, ArrayCount, 1);
		}

		//void ColorBuffer::GenerateMipMaps(CommandContext& BaseContext)
		//{
		//	if (m_NumMipMaps == 0)
		//		return;
		//
		//	ComputeContext& Context = BaseContext.GetComputeContext();
		//
		//	Context.SetRootSignature(Graphics::g_CommonRS);
		//
		//	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		//	Context.SetDynamicDescriptor(1, 0, m_SRVHandle);
		//
		//	for (uint32_t TopMip = 0; TopMip < m_NumMipMaps; )
		//	{
		//		uint32_t SrcWidth = m_Width >> TopMip;
		//		uint32_t SrcHeight = m_Height >> TopMip;
		//		uint32_t DstWidth = SrcWidth >> 1;
		//		uint32_t DstHeight = SrcHeight >> 1;
		//
		//		// Determine if the first downsample is more than 2:1.  This happens whenever
		//		// the source width or height is odd.
		//		uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
		//		if (m_Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
		//			Context.SetPipelineState(Graphics::g_GenerateMipsGammaPSO[NonPowerOfTwo]);
		//		else
		//			Context.SetPipelineState(Graphics::g_GenerateMipsLinearPSO[NonPowerOfTwo]);
		//
		//		// We can downsample up to four times, but if the ratio between levels is not
		//		// exactly 2:1, we have to shift our blend weights, which gets complicated or
		//		// expensive.  Maybe we can update the code later to compute sample weights for
		//		// each successive downsample.  We use _BitScanForward to count number of zeros
		//		// in the low bits.  Zeros indicate we can divide by two without truncating.
		//		uint32_t AdditionalMips;
		//		_BitScanForward((unsigned long*)&AdditionalMips,
		//			(DstWidth == 1 ? DstHeight : DstWidth) | (DstHeight == 1 ? DstWidth : DstHeight));
		//		uint32_t NumMips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
		//		if (TopMip + NumMips > m_NumMipMaps)
		//			NumMips = m_NumMipMaps - TopMip;
		//
		//		// These are clamped to 1 after computing additional mips because clamped
		//		// dimensions should not limit us from downsampling multiple times.  (E.g.
		//		// 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
		//		if (DstWidth == 0)
		//			DstWidth = 1;
		//		if (DstHeight == 0)
		//			DstHeight = 1;
		//
		//		Context.SetConstants(0, TopMip, NumMips, 1.0f / DstWidth, 1.0f / DstHeight);
		//		Context.SetDynamicDescriptors(2, 0, NumMips, m_UAVHandle + TopMip + 1);
		//		Context.Dispatch2D(DstWidth, DstHeight);
		//
		//		Context.InsertUAVBarrier(*this);
		//
		//		TopMip += NumMips;
		//	}
		//
		//	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		//		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		//}
	}


}