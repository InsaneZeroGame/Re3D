#pragma once
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
		std::vector<int> mIndices;
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
	struct Meshlet
	{
		std::vector<DirectX::XMFLOAT3> vertices;
		std::vector<uint32_t> indices;
		DirectX::XMFLOAT3 boundingSphereCenter;
		float boundingSphereRadius;
	};


	struct StaticMeshComponent : public Component
	{
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
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

		std::vector<Meshlet> ConvertToMeshlets(const std::vector<Renderer::Vertex>& vertices, const std::vector<int>& indices, size_t maxVerticesPerMeshlet, size_t maxIndicesPerMeshlet)
		{
			std::vector<Meshlet> meshlets;

			size_t vertexCount = vertices.size();
			size_t indexCount = indices.size();

			for (size_t i = 0; i < indexCount; i += maxIndicesPerMeshlet)
			{
				Meshlet meshlet;

				size_t currentIndexCount = std::min(maxIndicesPerMeshlet, indexCount - i);
				for (size_t j = 0; j < currentIndexCount; ++j)
				{
					uint32_t index = indices[i + j];
					meshlet.indices.push_back(index);
					meshlet.vertices.push_back(
						DirectX::XMFLOAT3(
							vertices[index].pos[0],
							vertices[index].pos[1],
							vertices[index].pos[2])
					);
				}

				// Calculate bounding sphere (simplified example)
				DirectX::XMFLOAT3 center = { 0.0f, 0.0f, 0.0f };
				float radius = 0.0f;
				for (const auto& vertex : meshlet.vertices)
				{
					center.x += vertex.x;
					center.y += vertex.y;
					center.z += vertex.z;
				}
				center.x /= meshlet.vertices.size();
				center.y /= meshlet.vertices.size();
				center.z /= meshlet.vertices.size();

				for (const auto& vertex : meshlet.vertices)
				{
					float distance = sqrtf((vertex.x - center.x) * (vertex.x - center.x) +
						(vertex.y - center.y) * (vertex.y - center.y) +
						(vertex.z - center.z) * (vertex.z - center.z));
					radius = std::max(radius, distance);
				}

				meshlet.boundingSphereCenter = center;
				meshlet.boundingSphereRadius = radius;

				meshlets.push_back(meshlet);
			}

			return meshlets;
		}

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