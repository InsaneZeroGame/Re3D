#pragma once
#include <DirectXMesh.h>

using ROTATION_AXIS = DirectX::SimpleMath::Vector3;
const ROTATION_AXIS X_AXIS = DirectX::SimpleMath::Vector3(1.0f,0.0f,0.0f);
const ROTATION_AXIS Y_AXIS = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
const ROTATION_AXIS Z_AXIS = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
constexpr int ROOT_PARA_FRAME_DATA_CBV = 0;
constexpr int ROOT_PARA_FRAME_SOURCE_TABLE = 1;//lights,clusters
constexpr int ROOT_PARA_COMPONENT_DATA = 2;
constexpr int ROOT_PARA_DIFFUSE_COLOR_TEXTURE = 3;
constexpr int ROOT_PARA_SHADOW_MAP = 4;
constexpr int ROOT_PARA_NORMAL_MAP_TEXTURE = 5;

namespace ECS
{
	//inline entt::registry gRegistry;
	using MaterialIndex = int;
	using MaterialName = std::string;
	struct SubMesh 
	{
		int IndexOffset = 0;
		int TriangleCount = 0;
        int IndexCount = 0;
	};
   

	//Mesh and transform info.
	struct StaticMesh
	{
		std::vector<Renderer::Vertex> mVertices;
		std::vector<uint32_t> mIndices;
        //std::vector<SubMesh> mSubmeshMap;
        std::unordered_map<MaterialIndex, SubMesh> mSubmeshMap;
		std::unordered_map<MaterialIndex, std::string> mMatBaseColorName;
		std::unordered_map<MaterialIndex, std::string> mMatNormalMapName;
		//std::unordered_map<MaterialName, SubMesh> mSubmeshMap;
		DirectX::XMFLOAT3 mDiffuseColor;
        bool mHasNormal;
        bool mHasUV;
        bool mAllByControlPoint;
		bool mHasTangent;
		bool mHasBitangent;
        DirectX::SimpleMath::Vector3 Rotation;
        DirectX::SimpleMath::Vector3 Scale;
        DirectX::SimpleMath::Vector3 Translation;
		std::string mName;
	};

	struct LigthData
	{
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 radius_attenu;
	};

	struct Component 
	{
        std::string mName;
	};

	// Meshlet structure
    static constexpr size_t MAX_VERTICES = 64; // Example size
    static constexpr size_t MAX_INDICES = 126; // Example size
	//struct MeshletInternal
	//{
    //    //std::array<DirectX::XMFLOAT3, MAX_VERTICES> vertices;
    //    //std::array<uint32_t, MAX_INDICES> indices;
    //    uint32_t vertexCount = 0;
    //    uint32_t primitiveCount = 0;
    //    DirectX::XMFLOAT3 boundingSphereCenter;
    //    float boundingSphereRadius;
	//};


	struct StaticMeshComponent : public Component
	{
		
		std::vector<Renderer::Vertex> mVertices;
		std::vector<uint32_t> mIndices;
		UINT mVertexCount;
		UINT mIndexCount;
		UINT StartIndexLocation;
		INT BaseVertexLocation;
        std::unordered_map<MaterialIndex, SubMesh> mSubMeshes;
		std::unordered_map<MaterialIndex, std::string> mMatBaseColorName;
		std::unordered_map<MaterialIndex, std::string> mMatNormalMapName;
		StaticMeshComponent(StaticMesh&& InMesh);
		DirectX::XMFLOAT3 mBaseColor;
		MaterialName MatName;
		MaterialName NormalMap;
		std::string mName;
		uint32_t mMeshletOffset = 0;

		std::vector<DirectX::Meshlet> mMeshlets;
		std::vector<DirectX::XMFLOAT3> mMeshletsVertices;
		std::vector<DirectX::MeshletTriangle> mMeshletPrimditives;
		std::vector<uint32_t> mMeshletsIndices;
		HRESULT ConvertToMeshlets(size_t maxVerticesPerMeshlet, size_t maxIndicesPerMeshlet);


		// New member variables for resources and SRVs
		ID3D12Resource* mMeshletsBuffer;
		ID3D12Resource* mMeshletsVerticesBuffer;
		ID3D12Resource* mMeshletsIndicesBuffer;
		ID3D12Resource* mMeshletsPrimitivesBuffer;

		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsVerticesSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsIndicesSRV;
		D3D12_CPU_DESCRIPTOR_HANDLE mMeshletsPrimitivesSRV;

		HRESULT CreateResources(ID3D12Device* device);
		HRESULT CreateSRVs(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvHeapHandle);

	};

	struct LightComponent : public Component
	{
		//std::array<float, 4> color;
		//std::array<float, 4> pos;
		//std::array<float, 4> radius_attenu;
		DirectX::XMFLOAT4 color;
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 radius_attenu;
	};

	struct TransformComponent : public Component
	{
        TransformComponent(StaticMesh&& InMesh);
        const DirectX::SimpleMath::Matrix GetModelMatrix(bool UploadToGpu = true);
        void Translate(const DirectX::SimpleMath::Vector3& InTranslate);
        void Scale(const DirectX::SimpleMath::Vector3& InScale);
        void Rotate(const DirectX::SimpleMath::Vector3& InAngles);
        DirectX::SimpleMath::Vector3& GetTranslate();
        DirectX::SimpleMath::Vector3& GetScale();
        DirectX::SimpleMath::Vector3& GetRotation();

    private:
        DirectX::SimpleMath::Vector3 mRotation;
        DirectX::SimpleMath::Vector3 mScale;
        DirectX::SimpleMath::Vector3 mTranslation;
        DirectX::SimpleMath::Matrix mMat = {};
	};

	class System
	{
	public:
		System();
		virtual ~System();

	protected:
	};
}