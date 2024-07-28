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
				: m_ClearColor(ClearColor), m_NumMipMaps(0), m_SampleCount(1)
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
			const D3D12_GPU_DESCRIPTOR_HANDLE& GetSRVGPU(void) const { return m_SRVGPUHandle; }
			const D3D12_GPU_DESCRIPTOR_HANDLE& GetRTVGPU(void) const { return m_RTVGPUHandle; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

			void SetClearColor(Color ClearColor) { m_ClearColor = ClearColor; }

			void SetMsaaMode(uint32_t NumColorSamples)
			{
				m_SampleCount = NumColorSamples;
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

				if (Flags == D3D12_RESOURCE_FLAG_NONE && m_SampleCount == 1)
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
			D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGPUHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_RTVGPUHandle;
			uint32_t m_NumMipMaps; // number of texture sublevels
			uint32_t m_SampleCount;
			//uint32_t m_SampleCount;
		};

		class UploadBuffer : public GpuResource
		{
		public:
			virtual ~UploadBuffer() { Destroy(); }

			void Create(const std::wstring& name, size_t BufferSize);

			size_t GetBufferSize() const { return m_BufferSize; }
			template<class T>
			size_t UploadData(std::span<T> InData)
			{
				auto sizeToUpload = InData.size_bytes();
				Ensures(m_BufferSize >= sizeToUpload + mOffset);
				memcpy(Map() + mOffset, InData.data(), sizeToUpload);
				Unmap();
				mOffset += sizeToUpload;
				return mOffset;
			};
			template<class T>
			void UpdataData(const T& InData)
			{
				auto sizeToUpload = sizeof(T);
				Ensures(m_BufferSize >= sizeToUpload);
				memcpy(Map(), &InData, sizeToUpload);
				Unmap();
			}
			size_t GetOffset() const { return mOffset; }
		protected:

			size_t m_BufferSize;

			size_t mOffset;

			uint8_t* Map(void);

			void Unmap(size_t begin = 0, size_t end = -1);

		};

		class GpuBuffer : public GpuResource
		{
		public:
			virtual ~GpuBuffer() { Destroy(); }

			// Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
			void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON,
				const void* initialData = nullptr);

			void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize,
				const UploadBuffer& srcData, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON, uint32_t srcOffset = 0);
			
			// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
			void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON,
				const void* initialData = nullptr);

			const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAV; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRV; }

			const D3D12_GPU_DESCRIPTOR_HANDLE& GetUAVGpu(void) const { return m_UAVGpu; }
			const D3D12_GPU_DESCRIPTOR_HANDLE& GetSRVGpu(void) const { return m_SRVGpu; }

			D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView(void) const { return m_GpuVirtualAddress; }

			D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

			D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
			D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
			{
				size_t Offset = BaseVertexIndex * m_ElementSize;
				return VertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize);
			}

			D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;
			D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
			{
				size_t Offset = StartIndex * m_ElementSize;
				return IndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), true);
			}

			size_t GetBufferSize() const { return m_BufferSize; }
			uint32_t GetElementCount() const { return m_ElementCount; }
			uint32_t GetElementSize() const { return m_ElementSize; }

			void SetUAV(D3D12_CPU_DESCRIPTOR_HANDLE InView) { m_UAV = InView; };
			void SetSRV(D3D12_CPU_DESCRIPTOR_HANDLE InView) { m_SRV = InView; };

		protected:

			GpuBuffer(void) : m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
			{
				m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			}

			D3D12_RESOURCE_DESC DescribeBuffer(void);
			virtual void CreateDerivedViews(void) = 0;

			D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
			D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;
			D3D12_GPU_DESCRIPTOR_HANDLE m_UAVGpu;
			D3D12_GPU_DESCRIPTOR_HANDLE m_SRVGpu;

			size_t m_BufferSize;
			uint32_t m_ElementCount;
			uint32_t m_ElementSize;
			D3D12_RESOURCE_FLAGS m_ResourceFlags;
		};

		inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
		{
			D3D12_VERTEX_BUFFER_VIEW VBView;
			VBView.BufferLocation = m_GpuVirtualAddress + Offset;
			VBView.SizeInBytes = Size;
			VBView.StrideInBytes = Stride;
			return VBView;
		}

		inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
		{
			D3D12_INDEX_BUFFER_VIEW IBView;
			IBView.BufferLocation = m_GpuVirtualAddress + Offset;
			IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			IBView.SizeInBytes = Size;
			return IBView;
		}

		class VertexBuffer final: public GpuBuffer
		{
		public:
			VertexBuffer(void)
			{
			}
			virtual void CreateDerivedViews(void) override {};

		};

		class ByteAddressBuffer : public GpuBuffer
		{
		public:
			virtual void CreateDerivedViews(void) override;
		};

		class IndirectArgsBuffer : public ByteAddressBuffer
		{
		public:
			IndirectArgsBuffer(void)
			{
			}
		};

		class StructuredBuffer : public GpuBuffer
		{
		public:
			virtual void Destroy(void) override
			{
				m_CounterBuffer.Destroy();
				GpuBuffer::Destroy();
			}

			virtual void CreateDerivedViews(void) override;
			ByteAddressBuffer& GetCounterBuffer(void) { return m_CounterBuffer; }
		private:
			ByteAddressBuffer m_CounterBuffer;
		};

		class TypedBuffer : public GpuBuffer
		{
		public:
			TypedBuffer(DXGI_FORMAT Format) : m_DataFormat(Format) {}
			virtual void CreateDerivedViews(void) override;

		protected:
			DXGI_FORMAT m_DataFormat;
		};

		class DepthBuffer : public ColorBuffer
		{
		public:
			DepthBuffer(float ClearDepth = 0.0f, uint8_t ClearStencil = 0)
				: m_ClearDepth(ClearDepth), m_ClearStencil(ClearStencil)
			{
				m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
				m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			}

			// Create a depth buffer.  If an address is supplied, memory will not be allocated.
			// The vmem address allows you to alias buffers (which can be especially useful for
			// reusing ESRAM across a frame.)
			void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
				D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
			void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
				D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
			// Get pre-created CPU-visible descriptor handles
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return m_hDSV[0]; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const { return m_hDSV[1]; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const { return m_hDSV[2]; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const { return m_hDSV[3]; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const { return m_hDepthSRV; }
			const D3D12_GPU_DESCRIPTOR_HANDLE& GetDepthSRVGPU() const { return m_hDepthSRVGPU; }
			const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const { return m_hStencilSRV; }

			float GetClearDepth() const { return m_ClearDepth; }
			uint8_t GetClearStencil() const { return m_ClearStencil; }

		protected:

			void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format);

			float m_ClearDepth;
			uint8_t m_ClearStencil;
			D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
			D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
			D3D12_GPU_DESCRIPTOR_HANDLE m_hDepthSRVGPU;
			D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
		};

		class Texture : public GpuResource
		{

		public:

			Texture() { m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
			Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle) : m_hCpuDescriptorHandle(Handle) {}

			// Create a 1-level textures
			void Create2D(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitData);
			void CreateCube(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void* InitialData);

			void CreateTGAFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
			bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
			bool CreateDDSFromFile(std::string InFileName,ID3D12CommandQueue* InCmdQueue, bool sRGB);
			void CreatePIXImageFromMemory(const void* memBuffer, size_t fileSize);

			virtual void Destroy() override
			{
				GpuResource::Destroy();
				m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
			}

			const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_hCpuDescriptorHandle; }
			const D3D12_GPU_DESCRIPTOR_HANDLE& GetSRVGpu() const { return m_hGpuDescriptorHandle; }

			uint32_t GetWidth() const { return m_Width; }
			uint32_t GetHeight() const { return m_Height; }
			uint32_t GetDepth() const { return m_Depth; }

		protected:

			uint32_t m_Width;
			uint32_t m_Height;
			uint32_t m_Depth;

			D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
			D3D12_GPU_DESCRIPTOR_HANDLE m_hGpuDescriptorHandle;

		};
	}
}
