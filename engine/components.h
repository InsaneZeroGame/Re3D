#pragma once

using ROTATION_AXIS = DirectX::SimpleMath::Vector3;
const ROTATION_AXIS X_AXIS = DirectX::SimpleMath::Vector3(1.0f,0.0f,0.0f);
const ROTATION_AXIS Y_AXIS = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
const ROTATION_AXIS Z_AXIS = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);


namespace ECS
{
	//inline entt::registry gRegistry;

	struct SubMesh 
	{
		int IndexOffset = 0;
		int TriangleCount = 0;
        int IndexCount = 0;
	};
    using MaterialIndex = int;

	//Mesh and transform info.
	struct StaticMesh
	{
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
        //std::vector<SubMesh> mSubmeshMap;
        std::unordered_map<MaterialIndex, SubMesh> mSubmeshMap;
        bool mHasNormal;
        bool mHasUV;
        bool mAllByControlPoint;
        DirectX::SimpleMath::Vector3 Rotation;
        DirectX::SimpleMath::Vector3 Scale;
        DirectX::SimpleMath::Vector3 Translation;
	};

	struct Component 
	{
        std::string mName;
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
		//StaticMeshComponent(std::vector<Renderer::Vertex>&& Vertices, std::vector<int>&& Indices);
		StaticMeshComponent(StaticMesh&& InMesh);
		//StaticMeshComponent(const StaticMeshComponent&) = delete;
		//StaticMeshComponent& operator=(const StaticMeshComponent&) = delete;
	};

	struct LightComponent : public Component
	{
		std::array<float, 4> color;
		std::array<float, 4> pos;
		std::array<float, 4> radius_attenu;
	};

	struct TransformComponent : public Component
	{
        TransformComponent(StaticMesh&& InMesh);
        const DirectX::SimpleMath::Matrix& GetModelMatrix(bool UploadToGpu = true);
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