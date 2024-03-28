#pragma once


namespace ECS
{
	//inline entt::registry gRegistry;

	struct StaticMesh
	{
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
	};

	struct StaticMeshComponent
	{
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
		UINT mVertexCount;
		UINT mIndexCount;
		UINT StartIndexLocation;
		INT BaseVertexLocation;
		StaticMeshComponent(std::vector<Renderer::Vertex>&& Vertices, std::vector<int>&& Indices);
		StaticMeshComponent(StaticMesh&& InMesh);
		//StaticMeshComponent(const StaticMeshComponent&) = delete;
		//StaticMeshComponent& operator=(const StaticMeshComponent&) = delete;
	};

	struct LightComponent
	{
		std::array<float, 4> color;
		std::array<float, 4> pos;
		std::array<float, 4> radius_attenu;
	};

	class System
	{
	public:
		System();
		virtual ~System();

	protected:
	};
}