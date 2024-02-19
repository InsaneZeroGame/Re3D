#pragma once
#include "graphics_common.h"

namespace AssetLoader
{
	struct Mesh
	{	
		std::vector<Renderer::Vertex> mVertices;
		std::vector<int> mIndices;
	};

	struct ModelAsset
	{	
		std::vector<Mesh> mMeshes;
	};

	class ModelAssetLoader
	{
	public:
		ModelAssetLoader() {};

		virtual ~ModelAssetLoader() {};

		virtual std::optional<ModelAsset> LoadAssetFromFile(std::string InFileName) = 0;

	private:

	};

	
}





