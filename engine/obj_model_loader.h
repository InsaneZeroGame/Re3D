#pragma once
#include "asset_loader.h"

namespace AssetLoader
{
	class ObjModelLoader final : public ModelAssetLoader
	{
	public:
		ObjModelLoader();

		~ObjModelLoader();

		std::vector<ECS::StaticMesh>& LoadAssetFromFile(std::string_view InFileName) override;

	private:
	};

}