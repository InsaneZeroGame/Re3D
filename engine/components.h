#pragma once


namespace ECS
{
	//inline entt::registry gRegistry;

	struct RenderComponent
	{
		struct Mesh
		{
			std::vector<Renderer::Vertex> mVertices;
			std::vector<int> mIndices;
			UINT mVertexCount;
			UINT mIndexCount;
			UINT StartIndexLocation;
			INT BaseVertexLocation;
		};

		std::vector<Mesh> mMeshes;
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