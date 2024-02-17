#pragma once

namespace Renderer
{
	namespace Resource
	{
		class GpuResource
		{
		public:
			GpuResource() :
				m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
				m_UsageState(D3D12_RESOURCE_STATE_COMMON),
				m_TransitioningState((D3D12_RESOURCE_STATES)-1)

			{
			}

			GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
				m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
				m_pResource(pResource),
				m_UsageState(CurrentState),
				m_TransitioningState((D3D12_RESOURCE_STATES)-1)
			{
			}

			~GpuResource() { Destroy(); }

			virtual void Destroy()
			{
				m_pResource = nullptr;
				m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
				++m_VersionID;
			}

			ID3D12Resource* operator->() { return m_pResource; }
			const ID3D12Resource* operator->() const { return m_pResource; }

			ID3D12Resource* GetResource() { return m_pResource; }
			const ID3D12Resource* GetResource() const { return m_pResource; }

			ID3D12Resource** GetAddressOf() { return &m_pResource; }

			D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

			uint32_t GetVersionID() const { return m_VersionID; }

		protected:

			ID3D12Resource* m_pResource;
			D3D12_RESOURCE_STATES m_UsageState;
			D3D12_RESOURCE_STATES m_TransitioningState;
			D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
			// Used to identify when a resource changes so descriptors can be copied etc.
			uint32_t m_VersionID = 0;
		};

		class PixelBuffer : public GpuResource
		{
		public:
			PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN){}

			uint32_t GetWidth(void) const { return m_Width; }
			uint32_t GetHeight(void) const { return m_Height; }
			uint32_t GetDepth(void) const { return m_ArraySize; }
			const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

			// Has no effect on Desktop
			void SetBankRotation(uint32_t RotationAmount)
			{
				(RotationAmount);
			}

			// Write the raw pixel buffer contents to a file
			// Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
			void ExportToFile(const std::wstring& FilePath);

		protected:

			D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

			void AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState);

			void CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
				D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

			static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
			static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
			static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
			static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
			static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
			static size_t BytesPerPixel(DXGI_FORMAT Format);

			uint32_t m_Width;
			uint32_t m_Height;
			uint32_t m_ArraySize;
			DXGI_FORMAT m_Format;
		};

		class ColorBuffer : public PixelBuffer
		{
		public:
			ColorBuffer(Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f))
				: m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
			{
				m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				for (int i = 0; i < _countof(m_UAVHandle); ++i)
					m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			}

			// Create a color buffer from a swap chain buffer.  Unordered access is restricted.
			void CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource);

			// Create a color buffer.  If an address is supplied, memory will not be allocated.
			// The vmem address allows you to alias buffers (which can be especially useful for
			// reusing ESRAM across a frame.)
			void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
				DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

			// Create a color buffer.  If an address is supplied, memory will not be allocated.
			// The vmem address allows you to alias buffers (which can be especially useful for
			// reusing ESRAM across a frame.)
			void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
				DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

			// Get pre-created CPU-visible descriptor handles
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRVHandle; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return m_RTVHandle; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

			void SetClearColor(Color ClearColor) { m_ClearColor = ClearColor; }

			void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
			{
				assert(NumCoverageSamples >= NumColorSamples);
				m_FragmentCount = NumColorSamples;
				m_SampleCount = NumCoverageSamples;
			}

			Color GetClearColor(void) const { return m_ClearColor; }

			// This will work for all texture sizes, but it's recommended for speed and quality
			// that you use dimensions with powers of two (but not necessarily square.)  Pass
			// 0 for ArrayCount to reserve space for mips at creation time.
			//void GenerateMipMaps(CommandContext& Context);

		protected:

			D3D12_RESOURCE_FLAGS CombineResourceFlags(void) const
			{
				D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

				if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
					Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

				return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
			}

			// Compute the number of texture levels needed to reduce to 1x1.  This uses
			// _BitScanReverse to find the highest set bit.  Each dimension reduces by
			// half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
			// as the dimension 511 (0x1FF).
			static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
			{
				uint32_t HighBit;
				_BitScanReverse((unsigned long*)&HighBit, Width | Height);
				return HighBit + 1;
			}

			void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

			Color m_ClearColor;
			D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
			uint32_t m_NumMipMaps; // number of texture sublevels
			uint32_t m_FragmentCount;
			uint32_t m_SampleCount;
		};


	}
}
